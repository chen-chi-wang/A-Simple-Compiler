#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "header.h"
#define NumsSize 23//EDITED2

int main( int argc, char *argv[] )
{
    FILE *source, *target;
    Program program;
//    SymbolTable symtab;
	HashMap *symmap;//EDITED2

    if( argc == 3){
        source = fopen(argv[1], "r");
        target = fopen(argv[2], "w");
        if( !source ){
            printf("can't open the source file\n");
            exit(2);
        }
        else if( !target ){
            printf("can't open the target file\n");
            exit(2);
        }
        else{
            program = parser(source);
            fclose(source);
            //symtab = build(program);
            symmap = mybuild(program);//EDITED2
            //check(&program, &symtab);
            mycheck(&program, symmap);//EDITED
//			puts("---------DEBUG----------");
//			fseek(source, 0, SEEK_SET);
//			test_parser(source);
//          fclose(source);
//			puts("\n---------DEBUG----------");
            gencode(program, target);
        }
    }
    else{
        printf("Usage: %s source_file target_file\n", argv[0]);
    }


    return 0;
}


/********************************************* 
  Scanning 
 *********************************************/
Token getNumericToken( FILE *source, char c )
{
    Token token;
    int i = 0;

    while( isdigit(c) ) {
        token.tok[i++] = c;
        c = fgetc(source);
    }

    if( c != '.' ){
        ungetc(c, source);
        token.tok[i] = '\0';
        token.type = IntValue;
        return token;
    }

    token.tok[i++] = '.';

    c = fgetc(source);
    if( !isdigit(c) ){
        ungetc(c, source);
        printf("Expect a digit : %c\n", c);
        exit(1);
    }

    while( isdigit(c) ){
        token.tok[i++] = c;
        c = fgetc(source);
    }

    ungetc(c, source);
    token.tok[i] = '\0';
    token.type = FloatValue;
    return token;
}

//EDITED2
Token getStringToken( FILE *source, char c )
{
    Token token;
    int i = 0;

    while( islower(c) ) {//TA
        token.tok[i++] = c;
        c = fgetc(source);
    }

    ungetc(c, source);
    token.tok[i] = '\0';
	token.type = Alphabet;
	return token;

}


Token scanner( FILE *source )
{
    char c;
    Token token;

    while( !feof(source) ){//why need?
        c = fgetc(source);

        while( isspace(c) ) c = fgetc(source);//c may be EOF, which is not space characer!

        if( isdigit(c) )
            return getNumericToken(source, c);
		
		token.tok[0] = c;
        token.tok[1] = '\0';
		switch(c){
                    case '=':
                            token.type = AssignmentOp;
                            return token;
                    case '+':
                            token.type = PlusOp;
                            return token;
                    case '-':
                            token.type = MinusOp;
                            return token;
                    case '*':
                            token.type = MulOp;
                            return token;
                    case '/':
                            token.type = DivOp;
                            return token;
                    case EOF:
                            token.type = EOFsymbol;
                            token.tok[0] = '\0';
                            return token;
                    default:
							if(!islower(c)){
								printf("Invalid character : %c\n", c);
	                            exit(1);
							}else{
								token = getStringToken(source, c);
								
							}
		} 
			

		if(strlen(token.tok)==1){
			if( c == 'f' )
                token.type = FloatDeclaration;
			else if( c == 'i' )
				token.type = IntegerDeclaration;
			else if( c == 'p' )
				token.type = PrintOp;
		}
		return token;

    }//end while

    token.tok[0] = '\0';
    token.type = EOFsymbol;
    return token;
}


/********************************************************
  Parsing
 *********************************************************/
Declaration parseDeclaration( FILE *source, Token token )
{
    Token token2;
    switch(token.type){
        case FloatDeclaration:
        case IntegerDeclaration:
            token2 = scanner(source);
			if(strlen(token2.tok)==1){//EDITED2
            	if (strcmp(token2.tok, "f") == 0 ||
                	    strcmp(token2.tok, "i") == 0 ||
                    	strcmp(token2.tok, "p") == 0) {
	                printf("Syntax Error: %s cannot be used as id\n", token2.tok);
    	            exit(1);
        	    }
			}
            return makeDeclarationNode( token, token2 );
        default:
            printf("Syntax Error: Expect Declaration %s\n", token.tok);
            exit(1);
    }
}

Declarations *parseDeclarations( FILE *source )
{
    Token token = scanner(source);
    Declaration decl;
    Declarations *decls;
    switch(token.type){
        case FloatDeclaration:
        case IntegerDeclaration:
            decl = parseDeclaration(source, token);
            decls = parseDeclarations(source);
            return makeDeclarationTree( decl, decls );
        case PrintOp:
        case Alphabet:
			ungets(token.tok, source);//EDITED2
//            ungetc(token.tok[0], source);
            return NULL;
        case EOFsymbol:
            return NULL;
        default:
            printf("Syntax Error: Expect declarations %s\n", token.tok);
            exit(1);
    }
}

//EDITED2
void ungets(char *str, FILE *source){
	int i, len = strlen(str);

	for(i = len -1; i >= 0; i--)
		ungetc(str[i], source);
}

Expression *parseValue( FILE *source )
{
    Token token = scanner(source);
    Expression *value = (Expression *)malloc( sizeof(Expression) );
    value->leftOperand = value->rightOperand = NULL;

    switch(token.type){
        case Alphabet:
            (value->v).type = Identifier;
            //(value->v).val.id = token.tok[0];
			memcpy((value->v).val.id, token.tok, strlen(token.tok)+1);//EDITED2
            break;
        case IntValue:
            (value->v).type = IntConst;
            (value->v).val.ivalue = atoi(token.tok);
            break;
        case FloatValue:
            (value->v).type = FloatConst;
            (value->v).val.fvalue = atof(token.tok);
            break;
        default:
            printf("Syntax Error: Expect Identifier or a Number %s\n", token.tok);
            exit(1);
    }

    return value;
}

Expression *parseExpressionTail( FILE *source, Expression *lvalue )
{
    Token token = scanner(source);
    Expression *expr;

    switch(token.type){
        case PlusOp:
            expr = (Expression *)malloc( sizeof(Expression) );
            (expr->v).type = PlusNode;
            (expr->v).val.op = Plus;
            expr->leftOperand = lvalue;
            expr->rightOperand = parseValue(source);
            return parseExpressionTail(source, expr);
        case MinusOp:
            expr = (Expression *)malloc( sizeof(Expression) );
            (expr->v).type = MinusNode;
            (expr->v).val.op = Minus;
            expr->leftOperand = lvalue;
            expr->rightOperand = parseValue(source);
            return parseExpressionTail(source, expr);
        case Alphabet:
        case PrintOp:
            //ungetc(token.tok[0], source);
            ungets(token.tok, source);//EDITED2
            return lvalue;
        case EOFsymbol:
            return lvalue;
        default:
            printf("Syntax Error: Expect a numeric value or an identifier %s\n", token.tok);
            exit(1);
    }
}

Expression *parseExpressionPlus( FILE *source, Expression *lvalue )
{
    Token token = scanner(source);
    Expression *expr;

    switch(token.type){
        case MulOp:
            expr = (Expression *)malloc( sizeof(Expression) );
            (expr->v).type = MulNode;
            (expr->v).val.op = Mul;
            expr->leftOperand = lvalue;
            expr->rightOperand = parseValue(source);
            return parseExpressionPlus(source, expr);
        case DivOp:
            expr = (Expression *)malloc( sizeof(Expression) );
            (expr->v).type = DivNode;
            (expr->v).val.op = Div;
            expr->leftOperand = lvalue;
            expr->rightOperand = parseValue(source);
            return parseExpressionPlus(source, expr);
        case PlusOp:
            ungetc(token.tok[0], source);
            return lvalue;
        case MinusOp:
            ungetc(token.tok[0], source);
            return lvalue;
        case Alphabet:
        case PrintOp:
            //ungetc(token.tok[0], source);
            ungets(token.tok, source);//EDITED2
			
            return lvalue;
        case EOFsymbol:
            return lvalue;
        default:
            printf("Syntax Error: Expect a numeric value or an identifier %s\n", token.tok);
            exit(1);
    }
}


Expression *parseExpression( FILE *source, Expression *lvalue )
{
    Token token = scanner(source);
    Expression *expr, *value;

    switch(token.type){
		case MulOp:
            expr = (Expression *)malloc( sizeof(Expression) );
            (expr->v).type = MulNode;
            (expr->v).val.op = Mul;
            expr->leftOperand = lvalue;
            expr->rightOperand = parseValue(source);
            return parseExpression(source, expr);

		case DivOp:
            expr = (Expression *)malloc( sizeof(Expression) );
            (expr->v).type = DivNode;
            (expr->v).val.op = Div;
            expr->leftOperand = lvalue;
            expr->rightOperand = parseValue(source);
            return parseExpression(source, expr);

        case PlusOp:
            expr = (Expression *)malloc( sizeof(Expression) );
            (expr->v).type = PlusNode;
            (expr->v).val.op = Plus;
            expr->leftOperand = lvalue;
			value = parseValue(source);
			expr->rightOperand = parseExpressionPlus(source, value);
			return parseExpression(source, expr);
//            expr->rightOperand = parseValue(source);
//            return parseExpressionTail(source, expr);

        case MinusOp:
            expr = (Expression *)malloc( sizeof(Expression) );
            (expr->v).type = MinusNode;
            (expr->v).val.op = Minus;
            expr->leftOperand = lvalue;
			value = parseValue(source);
			expr->rightOperand = parseExpressionPlus(source, value);
			return parseExpression(source, expr);

//            expr->rightOperand = parseValue(source);
//            return parseExpressionTail(source, expr);

        case Alphabet:
        case PrintOp:
//            ungetc(token.tok[0], source);
            ungets(token.tok, source);//EDITED2
            return lvalue;
//            return NULL;
        case EOFsymbol:
            return lvalue;
//            return NULL;
        default:
            printf("Syntax Error: Expect a numeric value or an identifier %s\n", token.tok);
            exit(1);
    }
}

Statement parseStatement( FILE *source, Token token )
{
    Token next_token;
    Expression *value, *expr;
    switch(token.type){
        case Alphabet:
            next_token = scanner(source);
            if(next_token.type == AssignmentOp){
                value = parseValue(source);
                expr = parseExpression(source, value);
//                return makeAssignmentNode(token.tok[0], expr);
                return makeAssignmentNode(token.tok, expr);//EDITED
            }
            else{
                printf("Syntax Error: Expect an assignment op %s\n", next_token.tok);//cloven foot?!
                exit(1);
            }
        case PrintOp:
            next_token = scanner(source);
            if(next_token.type == Alphabet)
//                return makePrintNode(next_token.tok[0]);
                return makePrintNode(next_token.tok);//EDITED2
            else{
                printf("Syntax Error: Expect an identifier %s\n", next_token.tok);
                exit(1);
            }
            break;
        default:
            printf("Syntax Error: Expect a statement %s\n", token.tok);
            exit(1);
    }
}

Statements *parseStatements( FILE * source )
{

    Token token = scanner(source);
    Statement stmt;
    Statements *stmts;

    switch(token.type){
        case Alphabet:
        case PrintOp:
            stmt = parseStatement(source, token);
            stmts = parseStatements(source);
            return makeStatementTree(stmt , stmts);
        case EOFsymbol:
            return NULL;
        default:
            printf("Syntax Error: Expect statements %s\n", token.tok);
            exit(1);
    }
}


/*********************************************************************
  Build AST
 **********************************************************************/
Declaration makeDeclarationNode( Token declare_type, Token identifier )
{
    Declaration tree_node;

    switch(declare_type.type){
        case FloatDeclaration:
            tree_node.type = Float;
            break;
        case IntegerDeclaration:
            tree_node.type = Int;
            break;
        default:
            break;
    }
//    tree_node.name = identifier.tok[0];
    memcpy(tree_node.name, identifier.tok, strlen(identifier.tok)+1);//EDITED2

    return tree_node;
}

Declarations *makeDeclarationTree( Declaration decl, Declarations *decls )
{
    Declarations *new_tree = (Declarations *)malloc( sizeof(Declarations) );
    new_tree->first = decl;
    new_tree->rest = decls;

    return new_tree;
}


//Statement makeAssignmentNode( char id, Expression *expr_tail )
Statement makeAssignmentNode( char *id, Expression *expr_tail )
{//EDITED2
    Statement stmt;
    AssignmentStatement assign;

    stmt.type = Assignment;
//    assign.id = id;
    memcpy(assign.id, id, strlen(id)+1);//EDITED2
    assign.expr = expr_tail;
    stmt.stmt.assign = assign;

    return stmt;
}

//Statement makePrintNode( char id )
//{
//    Statement stmt;
//    stmt.type = Print;
//    stmt.stmt.variable = id;
//
//    return stmt;
//}

//EDITED2
Statement makePrintNode( char *id )
{
    Statement stmt;
    stmt.type = Print;
	memcpy(stmt.stmt.variable, id, strlen(id)+1);//EDITED2

    return stmt;
}

Statements *makeStatementTree( Statement stmt, Statements *stmts )
{
    Statements *new_tree = (Statements *)malloc( sizeof(Statements) );
    new_tree->first = stmt;
    new_tree->rest = stmts;

    return new_tree;
}

/* parser */
Program parser( FILE *source )
{
    Program program;

    program.declarations = parseDeclarations(source);//makeDeclarationTree has malloc
    program.statements = parseStatements(source);//makeStatementTree has malloc

    return program;
}


/********************************************************
  Build symbol table
 *********************************************************/
void InitializeTable( SymbolTable *table )
{
    int i;

    for(i = 0 ; i < 26; i++)
        table->table[i] = Notype;
}

/* create and initialize hash map */ 
HashMap* InitializeMap(int size)
{
	/* create */	
	HashMap *map = calloc(1, sizeof(HashMap));
	map->size = size;
	map->storage = calloc(size, sizeof(HashNode*));//HashNode allocate!!//error: expected expression before ‘HashNode’

	/* initialize */
	int i;
	for(i = 0; i < size; i++){
		map->storage[i] = calloc(1, sizeof(HashNode));//important!! Modified. In order to initialize the value
//		map->storage[i]->key = calloc(65, sizeof(char));//BUG!!! don't forget to allocate space for it! (alternative solution)
		map->storage[i]->type = Notype;//map->storage[i] is a pointer to HashNode
	}
	return map;
}

//void add_table( SymbolTable *table, char c, DataType t )
//{
//    int index = (int)(c - 'a');
//
//    if(table->table[index] != Notype)
//        printf("Error : id %c has been declared\n", c);//error
//    table->table[index] = t;
//}

/* hash_set */
void add_map( HashMap *map, char* key, DataType t )
{
    int hashIdx = hash(map, key);
	
	//Since ASCII + array no collision,
	//however. it's normal that collsion happened in hash,
	//so we need to come up with new ideas!
	HashNode *node;
	while(map->storage[hashIdx]->type != Notype){//important!! Modified

		if(strcmp(map->storage[hashIdx]->key, key) == 0){
    	    printf("Error : id %s has been declared\n", key);//error
			break;//original idea
		}else{//collision
			if(hashIdx <= map->size-2){
				hashIdx++;
			}else{
				hashIdx = 0;
			}
		}
	}
	/* we can use this new node */
	memcpy(map->storage[hashIdx]->key, key, strlen(key)+1);//BUG!!!
    map->storage[hashIdx]->type = t;
}

int hash(HashMap *map, char *key)
{
	unsigned long hashval = 5381;//https://stackoverflow.com/questions/7666509/hash-function-for-string
	int c;

	while(c = *key++)
		hashval = ((hashval<<5) + hashval) + c;

	return hashval % map->size;//https://gist.github.com/tonious/1377667

}
//SymbolTable build( Program program )
//{
//    SymbolTable table;
//    Declarations *decls = program.declarations;
//    Declaration current;
//
//    InitializeTable(&table);
//
//    while(decls !=NULL){
//        current = decls->first;
//        add_table(&table, current.name, current.type);
//        decls = decls->rest;
//    }
//
//    return table;
//}

//EDITED2
HashMap* mybuild( Program program )
{
	HashMap* map;
    Declarations *decls = program.declarations;
    Declaration current;

    map = InitializeMap(NumsSize * 2);

    while(decls !=NULL){
        current = decls->first;
        add_map(map, current.name, current.type);
        decls = decls->rest;
    }

    return map;
}



/********************************************************************
  Type checking
 *********************************************************************/

//void convertType( Expression * old, DataType type )
//{
//    if(old->type == Float && type == Int){
//        printf("error : can't convert float to integer\n");
//        return;
//    }
//    if(old->type == Int && type == Float){
//        Expression *tmp = (Expression *)malloc( sizeof(Expression) );
//        if(old->v.type == Identifier)
//            printf("convert to float %c \n",old->v.val.id);
//        else
//            printf("convert to float %d \n", old->v.val.ivalue);
//        tmp->v = old->v;
//        tmp->leftOperand = old->leftOperand;
//        tmp->rightOperand = old->rightOperand;
//        tmp->type = old->type;
//
//        Value v;
//        v.type = IntToFloatConvertNode;
//        v.val.op = IntToFloatConvert;
//        old->v = v;
//        old->type = Int;
//        old->leftOperand = tmp;
//        old->rightOperand = NULL;
//    }
//}

//EDITED3
bool isConvertType( Expression * old, DataType type )
{
    if(old->type == Float && type == Int){
        printf("error : can't convert float to integer\n");
//        return;
        return false;//EDITED3
    }
    if(old->type == Int && type == Float){
        Expression *tmp = (Expression *)malloc( sizeof(Expression) );
        if(old->v.type == Identifier)
//            printf("convert to float %c \n",old->v.val.id);
            printf("convert to float %s \n",old->v.val.id);//EDITED2
        else
            printf("convert to float %d \n", old->v.val.ivalue);
        tmp->v = old->v;
        tmp->leftOperand = old->leftOperand;
        tmp->rightOperand = old->rightOperand;
        tmp->type = old->type;

        Value v;
        v.type = IntToFloatConvertNode;
        v.val.op = IntToFloatConvert;
        old->v = v;
        old->type = Int;
        old->leftOperand = tmp;
        old->rightOperand = NULL;
		return true;//EDITED3
    }
	return false;//EDITED3
}


DataType generalize( Expression *left, Expression *right )
{
    if(left->type == Float || right->type == Float){
        printf("generalize : float\n");
        return Float;
    }
    printf("generalize : int\n");
    return Int;
}

//DataType lookup_table( SymbolTable *table, char c )
//{//pass by address for speed!
//    int id = c-'a';
//    if( table->table[id] != Int && table->table[id] != Float)
//        printf("Error : identifier %c is not declared\n", c);//error
//    return table->table[id];
//}

//HARD!!
/*hash_get*/
DataType lookup_map( HashMap *map, char *key )
{
	int hashIdx = hash(map, key);
	/*important!!*/
	HashNode *node;
	while(map->storage[hashIdx]->type != Notype){//collision is not the issue here? No! important!!
		node = map->storage[hashIdx];//has node
		if(strcmp(node->key, key)==0){
//			if(node->type != Int && node->type != Float) 
//        		printf("Error : identifier %c is not declared\n", c);//error
			return node->type;		
		}else{
			if(hashIdx<=map->size-2){
				hashIdx++;
			}else{
				hashIdx = 0;
			}
		}
	}	
	printf("Error : identifier %s is not declared\n", key);//error
    return Notype;//not correct, here is hash_get!
}

//void checkexpression( Expression * expr, SymbolTable * table )
//{
//    char c;
//    if(expr->leftOperand == NULL && expr->rightOperand == NULL){
//        switch(expr->v.type){
//            case Identifier:
//                c = expr->v.val.id;
//                printf("identifier : %c\n",c);
//                expr->type = lookup_table(table, c);
//                break;
//            case IntConst:
//                printf("constant : int\n");
//                expr->type = Int;
//                break;
//            case FloatConst:
//                printf("constant : float\n");
//                expr->type = Float;
//                break;
//                //case PlusNode: case MinusNode: case MulNode: case DivNode:
//            default:
//                break;
//        }
//    }
//    else{
//        Expression *left = expr->leftOperand;
//        Expression *right = expr->rightOperand;
//
//        checkexpression(left, table);
//        checkexpression(right, table);
//
//        DataType type = generalize(left, right);
//        convertType(left, type);//left->type = type;//converto
//        convertType(right, type);//right->type = type;//converto
//        expr->type = type;
//    }
//}

//EDITED3
void mycheckexpression( Expression * expr, HashMap *map )
{
    char str[65];//EDITED2
    if(expr->leftOperand == NULL && expr->rightOperand == NULL){
        switch(expr->v.type){
            case Identifier:
                memcpy(str, expr->v.val.id, strlen(expr->v.val.id)+1);//EDITED2
                printf("identifier : %s\n",str);//EDITED2
                expr->type = lookup_map(map, str);//EDITED2
                break;
            case IntConst:
                printf("constant : int\n");
                expr->type = Int;
                break;
            case FloatConst:
                printf("constant : float\n");
                expr->type = Float;
                break;
                //case PlusNode: case MinusNode: case MulNode: case DivNode:
            default:
                break;
        }
    }
    else{
        Expression *left = expr->leftOperand;
        Expression *right = expr->rightOperand;

        mycheckexpression(left, map);
        mycheckexpression(right, map);

        DataType type = generalize(left, right);
		expr->type = type;

		/*Constant Folding Optimization*/
		bool lFlag = false, rFlag = false;//EDITED3
//		convertType(left, type);//left->type = type;
//		convertType(right, type);//right->type = type;

		lFlag = isConvertType(left, type);//left->type = type;//EDITED3
		rFlag = isConvertType(right, type);//right->type = type;//EDITED3

		if(lFlag==true || rFlag==true){
			if(lFlag && left->leftOperand->v.type == IntConst && right->v.type == FloatConst){
				calculate_op(expr, lFlag, rFlag); 
				expr->v.type = FloatConst;
				expr->leftOperand = NULL;
				expr->rightOperand = NULL;
			}
			if(rFlag && right->leftOperand->v.type == IntConst && left->v.type == FloatConst){
				calculate_op(expr, lFlag, rFlag); 
				expr->v.type = FloatConst;
				expr->leftOperand = NULL;
				expr->rightOperand = NULL;

			}
		}else{
			if(left->v.type == IntConst && right->v.type == IntConst){
				calculate_op(expr, lFlag, rFlag); 
				expr->v.type = IntConst;
				expr->leftOperand = NULL;
				expr->rightOperand = NULL;

			}
			if(left->v.type == FloatConst && right->v.type == FloatConst){
				calculate_op(expr, lFlag, rFlag); 
				expr->v.type = FloatConst;
				expr->leftOperand = NULL;
				expr->rightOperand = NULL;
			}
		}
    }
}

//void checkstmt( Statement *stmt, SymbolTable * table )
//{
//    if(stmt->type == Assignment){
//        AssignmentStatement assign = stmt->stmt.assign;
//        printf("assignment : %c \n",assign.id);
//        checkexpression(assign.expr, table);
//        stmt->stmt.assign.type = lookup_table(table, assign.id);
//        if (assign.expr->type == Float && stmt->stmt.assign.type == Int) {
//            printf("error : can't convert float to integer\n");
//        } else {
//            convertType(assign.expr, stmt->stmt.assign.type);
//        }
//    }
//    else if (stmt->type == Print){
//        printf("print : %c \n",stmt->stmt.variable);
//        lookup_table(table, stmt->stmt.variable);
//    }
//    else printf("error : statement error\n");//error
//}

//EDITED
void mycheckstmt( Statement *stmt, HashMap * map )
{
    if(stmt->type == Assignment){
        AssignmentStatement assign = stmt->stmt.assign;
        printf("assignment : %s \n",assign.id);//EDITED2
        mycheckexpression(assign.expr, map);
        stmt->stmt.assign.type = lookup_map(map, assign.id);
        if (assign.expr->type == Float && stmt->stmt.assign.type == Int) {
            printf("error : can't convert float to integer\n");
        } else {
            isConvertType(assign.expr, stmt->stmt.assign.type);//EDITED3
        }
    }
    else if (stmt->type == Print){
        printf("print : %s \n",stmt->stmt.variable);//EDITED2
        lookup_map(map, stmt->stmt.variable);
    }
    else printf("error : statement error\n");//error
}


//void check( Program *program, SymbolTable * table )
//{//pass table by address in order to speed up!
//    Statements *stmts = program->statements;
//    while(stmts != NULL){
//        checkstmt(&stmts->first,table);//notice & precedence
//        stmts = stmts->rest;
//    }
//}

void mycheck( Program *program, HashMap * map )
{
    Statements *stmts = program->statements;
    while(stmts != NULL){
        mycheckstmt(&stmts->first,map);
        stmts = stmts->rest;
    }
}


/***********************************************************************
  Code generation
 ************************************************************************/
void fprint_op( FILE *target, ValueType op )
{
    switch(op){
		case MulNode://EDITED1
            fprintf(target,"*\n");
            break;
        case DivNode://EDITED1
            fprintf(target,"/\n");
            break;
        case MinusNode:
            fprintf(target,"-\n");
            break;
        case PlusNode:
            fprintf(target,"+\n");
            break;
        default:
            fprintf(target,"Error in fprintf_op ValueType = %d\n",op);
            break;
    }
}

//EDITED3
void calculate_op( Expression *expr, bool lFlag, bool rFlag )
{
	Expression *left = expr->leftOperand;
	Expression *right = expr->rightOperand;
	bool isInt;
	if(expr->type==Int){
		isInt = true;
	}else if(expr->type==Float){
		isInt = false;
	}
    switch(expr->v.type){
		case MulNode://EDITED1
			if(!(lFlag^rFlag)){
				(isInt)?\
				(expr->v.val.ivalue = left->v.val.ivalue * right->v.val.ivalue):\
				(expr->v.val.fvalue = left->v.val.fvalue * right->v.val.fvalue); 
			}else if(lFlag){
				expr->v.val.fvalue = left->leftOperand->v.val.ivalue * right->v.val.fvalue;
			}else{
				expr->v.val.fvalue = left->v.val.fvalue * right->leftOperand->v.val.ivalue;
			}	
            break;
        case DivNode://EDITED1
			if(!(lFlag^rFlag)){
				(isInt)?\
				(expr->v.val.ivalue = left->v.val.ivalue / right->v.val.ivalue):\
				(expr->v.val.fvalue = left->v.val.fvalue / right->v.val.fvalue); 
			}else if(lFlag){
				expr->v.val.fvalue = left->leftOperand->v.val.ivalue / right->v.val.fvalue;
			}else{
				expr->v.val.fvalue = left->v.val.fvalue / right->leftOperand->v.val.ivalue;
			}	
            break;
        case MinusNode:
			if(!(lFlag^rFlag)){
				(isInt)?\
				(expr->v.val.ivalue = left->v.val.ivalue - right->v.val.ivalue):\
				(expr->v.val.fvalue = left->v.val.fvalue - right->v.val.fvalue); 
			}else if(lFlag){
				expr->v.val.fvalue = left->leftOperand->v.val.ivalue - right->v.val.fvalue;
			}else{
				expr->v.val.fvalue = left->v.val.fvalue - right->leftOperand->v.val.ivalue;
			}	
            break;
        case PlusNode:
			if(!(lFlag^rFlag)){
				(isInt)?\
				(expr->v.val.ivalue = left->v.val.ivalue + right->v.val.ivalue):\
				(expr->v.val.fvalue = left->v.val.fvalue + right->v.val.fvalue); 
			}else if(lFlag){
				expr->v.val.fvalue = left->leftOperand->v.val.ivalue + right->v.val.fvalue;
			}else{
				expr->v.val.fvalue = left->v.val.fvalue + right->leftOperand->v.val.ivalue;
			}	
            break;
        default:
            printf("Error in calculate_op ValueType = %d\n",expr->v.type);
            break;
    }
}


void fprint_expr( FILE *target, Expression *expr)
{

    if(expr->leftOperand == NULL){
        switch( (expr->v).type ){
            case Identifier:
//                fprintf(target,"l%c\n",(expr->v).val.id);
                fprintf(target,"l%s\n",(expr->v).val.id);//EDITED2
                break;
            case IntConst:
                fprintf(target,"%d\n",(expr->v).val.ivalue);
                break;
            case FloatConst:
                fprintf(target,"%.1f\n", (expr->v).val.fvalue);
                break;
            default:
                fprintf(target,"Error In fprint_left_expr. (expr->v).type=%d\n",(expr->v).type);
                break;
        }
    }
    else{
        fprint_expr(target, expr->leftOperand);
        if(expr->rightOperand == NULL){
            fprintf(target,"5k\n");
        }
        else{
            //	fprint_right_expr(expr->rightOperand);
            fprint_expr(target, expr->rightOperand);
            fprint_op(target, (expr->v).type);
        }
    }
}

void gencode(Program prog, FILE * target)
{
    Statements *stmts = prog.statements;
    Statement stmt;

    while(stmts != NULL){
        stmt = stmts->first;
        switch(stmt.type){
            case Print:
                //fprintf(target,"l%c\n",stmt.stmt.variable);
                fprintf(target,"l%s\n",stmt.stmt.variable);//EDITED2
                fprintf(target,"p\n");
                break;
            case Assignment:
                fprint_expr(target, stmt.stmt.assign.expr);
                /*
                   if(stmt.stmt.assign.type == Int){
                   fprintf(target,"0 k\n");
                   }
                   else if(stmt.stmt.assign.type == Float){
                   fprintf(target,"5 k\n");
                   }*/
                //fprintf(target,"s%c\n",stmt.stmt.assign.id);
                fprintf(target,"s%s\n",stmt.stmt.assign.id);//EDITED2
                fprintf(target,"0 k\n");
                break;
        }
        stmts=stmts->rest;
    }

}


/***************************************
  For our debug,
  you can omit them.
 ****************************************/
void print_expr(Expression *expr)
{
    if(expr == NULL)
        return;
    else{
        print_expr(expr->leftOperand);
        switch((expr->v).type){
            case Identifier:
//                printf("%c ", (expr->v).val.id);
                printf("%s ", (expr->v).val.id);//EDITED2
                break;
            case IntConst:
                printf("%d ", (expr->v).val.ivalue);
                break;
            case FloatConst:
                printf("%f ", (expr->v).val.fvalue);
                break;
            case PlusNode:
                printf("+ ");
                break;
            case MinusNode:
                printf("- ");
                break;
            case MulNode:
                printf("* ");
                break;
            case DivNode:
                printf("/ ");
                break;
            case IntToFloatConvertNode:
                printf("(float) ");
                break;
            default:
                printf("error ");
                break;
        }
        print_expr(expr->rightOperand);
    }
}

void test_parser( FILE *source )
{
    Declarations *decls;
    Statements *stmts;
    Declaration decl;
    Statement stmt;
    Program program = parser(source);

    decls = program.declarations;

    while(decls != NULL){
        decl = decls->first;
        if(decl.type == Int)
            printf("i ");
        if(decl.type == Float)
            printf("f ");
//        printf("%c ",decl.name);
        printf("%s ",decl.name);//EDITED2
        decls = decls->rest;
    }

    stmts = program.statements;

    while(stmts != NULL){
        stmt = stmts->first;
        if(stmt.type == Print){
//            printf("p %c ", stmt.stmt.variable);
            printf("p %s ", stmt.stmt.variable);//EDITED2
        }

        if(stmt.type == Assignment){
//            printf("%c = ", stmt.stmt.assign.id);
            printf("%s = ", stmt.stmt.assign.id);//EDITED2
            print_expr(stmt.stmt.assign.expr);
        }
        stmts = stmts->rest;
    }

}
