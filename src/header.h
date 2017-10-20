#ifndef HEADER_H_INCLUDED
#define HEADER_H_INCLUDED

/******************************************************************************************************************************************
    All enumeration literals
       TokenType : Specify the type of the token scanner returns
	   DataType  : The data type of the declared variable
	   StmtType  : Indicate one statement in AcDc program is print or assignment statement.
	   ValueType : The node types of the expression tree that represents the expression on the right hand side of the assignment statement.
	               Identifier, IntConst, FloatConst must be the leaf nodes ex: a, b, c , 1.5 , 3.
				   PlusNode, MinusNode, MulNode, DivNode are the operations in AcDc. They must be the internal nodes.
                   Note that IntToFloatConvertNode to represent the type coercion may appear after finishing type checking. 			  
	   Operation : Specify all arithematic expression, including +, - , *, / and type coercion.
*******************************************************************************************************************************************/

typedef enum TokenType { FloatDeclaration, IntegerDeclaration, PrintOp, AssignmentOp, PlusOp, MinusOp,
             MulOp, DivOp, Alphabet, IntValue, FloatValue, EOFsymbol } TokenType;
typedef enum DataType { Int, Float, Notype }DataType;
typedef enum StmtType { Print, Assignment } StmtType;
typedef enum ValueType { Identifier, IntConst, FloatConst, PlusNode, MinusNode, MulNode, DivNode, IntToFloatConvertNode }ValueType;
typedef enum Operation { Plus, Minus, Mul, Div, Assign, IntToFloatConvert } Operation;

//EDITED3
typedef enum{ false, true }bool;//won't conflict with C99?!
	

/**************************************************************************************** 
   All structures to facilitate the processes of 
   scanning, parsing, AST, type-checking, building the symbol table, and code generation.
*****************************************************************************************/


/* For scanner */
typedef struct Token{
    TokenType type;
    char tok[1025];
}Token;

/*** The following are nodes of the AST. ***/

/* For decl production or say one declaration statement */
typedef struct Declaration{
    DataType type;
//    char name;
    char name[65];//EDITED2
}Declaration;

/* 
    For decls production or say all declarations. (
	You can view it as the subtree for decls in AST,
	or just view it as the linked list that stores 
	all declarations. ) 
*/
typedef struct Declarations{
    Declaration first;
    struct Declarations *rest;
}Declarations;

/* For the nodes of the expression on the right hand side of one assignment statement */
typedef struct Value{
    ValueType type;
    union{
        //char id;                   /* if the node represent the access of the identifier */
        char id[65];                 //EDITED2
        Operation op;              /* store +, -, *, /, =, type_convert */
        int ivalue;                /* for integer constant in the expression */
        float fvalue;              /* for float constant */
    }val;
}Value;


/* 
   The data structure of the expression tree.
   Recall how to deal with expression by tree 
   in data structure course.   
*/
typedef struct Expression{
    Value v;
    struct Expression *leftOperand;
    struct Expression *rightOperand;
    DataType type;
}Expression;


/* For one assignment statement */
typedef struct AssignmentStatement{
//    char id;
    char id[65];//EDITED2
    Expression *expr;
    DataType type;      /* For type checking to store the type of all expression on the right. */
}AssignmentStatement;


/* For stmt production or say one statement*/
typedef struct Statement{
    StmtType type;
    union{
//        char variable;              /* print statement */
        char variable[65];             //EDITED2
        AssignmentStatement assign;
    }stmt;
}Statement;

/* For stmts production or say all productions */
typedef struct Statements{
    struct Statement first;
    struct Statements *rest;
}Statements;

/* For the root of the AST. */
typedef struct Program{
    Declarations *declarations;
    Statements *statements;
}Program;

/* For building the symbol table */
typedef struct SymbolTable{
    DataType table[26];
}SymbolTable;

/* For building the symbol table */
typedef struct{
//	char *key;//BUG!! Also need to allocate space for it in InitializeMap step!!!
	char key[65];//BUG!! Also need to allocate space for it!!!
	DataType type;//value
}HashNode;

typedef struct{
	int size;
	HashNode* *storage;
}HashMap;


Token getNumericToken( FILE *source, char c );
Token getStringToken( FILE *source, char c );//EDITED2
Token scanner( FILE *source );
Declaration makeDeclarationNode( Token declare_type, Token identifier );
Declarations *makeDeclarationTree( Declaration decl, Declarations *decls );
Declaration parseDeclaration( FILE *source, Token token );
Declarations *parseDeclarations( FILE *source );
Expression *parseValue( FILE *source );
Expression *parseExpressionTail( FILE *source, Expression *lvalue );
Expression *parseExpressionPlus( FILE *source, Expression *lvalue );//EDITED1
Expression *parseExpression( FILE *source, Expression *lvalue );
//Statement makeAssignmentNode( char id, Expression *expr_tail );
Statement makeAssignmentNode( char *id, Expression *expr_tail );//EDITED
//Statement makePrintNode( char id );
Statement makePrintNode( char *id );//EDITED2
Statements *makeStatementTree( Statement stmt, Statements *stmts );
Statement parseStatement( FILE *source, Token token );
Statements *parseStatements( FILE * source );
Program parser( FILE *source );
void InitializeTable( SymbolTable *table );
HashMap* InitializeMap(int size);//EDITED2
void add_table( SymbolTable *table, char c, DataType t );
void add_map( HashMap *map, char* key, DataType t );//EDITED2
int hash(HashMap *map, char *key);//EDITED2
SymbolTable build( Program program );
HashMap* mybuild( Program program );//EDITED2
void convertType( Expression * old, DataType type );
bool isConvertType( Expression * old, DataType type );//EDITED3
DataType generalize( Expression *left, Expression *right );
DataType lookup_table( SymbolTable *table, char c );
DataType lookup_map( HashMap *map, char *key );//EDITED2
void checkexpression( Expression * expr, SymbolTable * table );
void mycheckexpression( Expression * expr, HashMap *map );//EDITED3
void checkstmt( Statement *stmt, SymbolTable * table );
void mycheckstmt( Statement *stmt, HashMap * map );//EDITED
void check( Program *program, SymbolTable * table);
void mycheck( Program *program, HashMap * map );//EDITED
void fprint_op( FILE *target, ValueType op );
void calculate_op( Expression *expr, bool lFlag, bool rFlag );//EDITED3
void fprint_expr( FILE *target, Expression *expr );
void gencode( Program prog, FILE * target );

void print_expr( Expression *expr );
void test_parser( FILE *source );

//EDITED2
void ungets(char *str, FILE *source);


#endif // HEADER_H_INCLUDED
