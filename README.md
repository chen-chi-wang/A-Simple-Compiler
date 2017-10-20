# A-Simple-Compiler

## Requirements
In this assignment, we are required to extend the AcDc compiler in three ways:

**1. Extend the AC language to accept integer multiply (*) and divide (/) operators.**

**2. The AC language supports only single character variable names. Relax this restriction for longer names (not exceed 64 characters, different variables will not exceed 23).**

**3. Enhance the AcDc compiler with a simple optimization called “constant folding”, which evaluates constant expressions at compile time.**

## Demo
```
cd src
make
./AcDc ../test/sample.ac output
```
**`sample.ac`** is an AC source code

**`output`** can be examined
- postorder traversal of the expressions (semantic tree)
- constant folding


## Task 1 : Extend for Multiply (*) and Divide (/) Operators

When parsing a statement, the compiler builds a parse tree for Syntax Analysis(語法分析).

<img src="https://i.imgur.com/y2DhZtO.png" width="280">

Here is the CFG (Context Free Grammar) for the assignment statement of AC language:
```
stmt -> id assign val expr
expr -> plus val expr
      | minus val expr
```
As you can see, the `expr` is a recursive grammar which make the RHS of assignment statement like this - `A { + B } { + C }...`, A, B, and C are value (Constant or Identifier).

Originally, AC language only accepts the (+) and (-) operators. We are now going to relax this restriction to accept multiply (*) and divide (/) operators.

### Algorithms
**Original Version**

Here provided the relative pseudo code of the original version:

ex: A = B + C - D
```clike
parseStatement{
    token = LHS;//A
    value = parseValue;//B
    expr = parseExpression(value);
    makeAssignmentNode(token, expr);
}
```
Function `parseStatement` decides what kind of statement is parsing. 

Here only show the assignment statement.

Function `parseValue` request a lexical token from scanner.

Scanner has parsed: 

<img src="https://i.imgur.com/U9AiCTx.png" width="100">

Tree has been built: 

<img src="https://i.imgur.com/Hy4N8u5.png" height="185">

```clike
parseExpression(lvalue){//B
    case PlusOp://+
        expr->leftOperand = lvalue;
        expr->leftOperand = parseValue;//C
        return parseExpression(expr);
}
```
Function `parseExpression` handles the recursive grammar of the left associative operator (i.e., `+` and `-`).

Scanner has parsed: 

<img src="https://i.imgur.com/EeUAhPk.png" width="100">

Tree has been built: 

<img src="https://i.imgur.com/5YtkdW1.png" height="185">

```clike
parseExpression(lvalue){//+ tree
    case MinusOp://-
        expr->leftOperand = lvalue;
        expr->rightOperand = parseValue;//D
        return parseExpression(expr);
    case Alphabet:
    case PrintOp:
            ungetc()
            return lvalue;
    case EOFsymbol:
            return lvalue;
}
```
Scanner has parsed: 

<img src="https://i.imgur.com/yrSG09T.png" width="100">

Tree has been built: 

<img src="https://i.imgur.com/Pdw0IuV.png" height="185">

```clike
parseExpression(lvalue){//- tree
    case MinusOp:
        expr->leftOperand = lvalue;
        expr->rightOperand = parseValue;
        return parseExpression(expr);
    case Alphabet:
    case PrintOp:
            ungetc()
            return lvalue;//return - tree
    case EOFsymbol:
            return lvalue;
}
```
Tree has been built: 

<img src="https://i.imgur.com/aDnEfVH.png" height="185">


---
**Prerequisite Knowledge**

- Since the precedence for `+` and `-` are equal, due to the left-to-right associativity of the `+`, `-`, `*`, `/` operators, the evaluation is from left-to-right.

- Notice that the precedence of `*` and `/` operators are higher than the `+` and `-` operators.

- The postorder traversal is useful to get the postfix expression, which is used to compute the numerical result.

**Some Observation**

Given `A = B + C ? D`

If `?` is `-`: `A = B + C - D`

<img src="https://i.imgur.com/WQgMSoj.png" height="200">

If `?` is `*` : `A = B + C * D`

<img src="https://i.imgur.com/WutEUVn.png" height="200">

**Conclusion** - The right operand of `+` operator need to be further handled:

a. <img src="https://i.imgur.com/mVrypax.png" height="200"> &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;b. <img src="https://i.imgur.com/O5ltOwL.png" height="200">

**Modification (Extended Version)**

Our modification is in the function `parseExpression`

```
parseExpression(lvalue){//C, + tree
    case MulOp:
        expr->leftOperand = lvalue
        expr->rightOperand = parseValue
        return parseExpression(expr)
    case PlusOp:
        expr->leftOperand = lvalue;
        value = parseValue;
        expr->rightOperand = parseExpressionPlus(value);
        return parseExpression(expr);
}
```
Here, we only show the `+` and `*` as representative for the two precedence groups.

Notice that the right operand of `+` operator need to be further handled.

**Newly Created Function**

Function `parseExpressionPlus` is a newly created function to handle the right operand of group `+`'s operators.

*Figure b's Case* - If the operand on the right hand side of `+` operator has a `*` operator on its right hand side, then the operand is belong to the left operand of that `*` operator. 

*Figure a's Case* - Otherwise, the operand on the right hand side of `+` operator is belong to that `+` operator.
```
parseExpressionPlus(lvalue){
    case MulOp:
        expr->leftOperand = lvalue
        expr->rightOperand = parseValue //figure b's case
        return parseExpressionPlus(expr)
    case PlusOp:    
        return lvalue;//figure a's case
}
```

## Task 2 : Relax Variable Names to String
The symbol table records the mapping between identifier/name and type (Int or Float) of a variable. The compiler builds the symbol table for two reasons:
1. **Multiple definitions check**
2. **Semantic analysis** - After the parsing phase, the compiler would build the symbol table according to the declaration statements, then use this table in the follwing semantic check.

<img src="https://i.imgur.com/V0U7hFw.png" width="280">

### Data Structures and Functions
Our modification explanation would follow the symbol table building steps. The first part illustrates the original single character version.

0. **Data Structure** - Originally, an array named `table` is used to stored the type (Int or Float) for each single character variable.
```c
/* For building the symbol table */
typedef struct SymbolTable{
    DataType table[26];
}SymbolTable;
```

1. **Initialize**

```c
void InitializeTable( SymbolTable *table )
{
    int i;

    for(i = 0 ; i < 26; i++)
        table->table[i] = Notype;
}
```

2. **Create** - The index of each character variable can be derived by subtracting a's ASCII. During the table building, check whether there is an "multiple difinition error".

```c
void add_table( SymbolTable *table, char c, DataType t )
{
    int index = (int)(c - 'a');

    if(table->table[index] != Notype)
        printf("Error : id %c has been declared\n", c);//error
    table->table[index] = t;
}
```

3. **Lookup** - Look up variable type during the semantic phase. Check whether there is an undeclared identifier. 
```c
DataType lookup_table( SymbolTable *table, char c )
{
    int id = c-'a';
    if( table->table[id] != Int && table->table[id] != Float)
        printf("Error : identifier %c is not declared\n", c);//error
    return table->table[id];
}
```
---
The second part illustrates the string variable names version.

0. **Data Structure** - I used hash data structure to store the type mapping for string variable. Variable's identifier/name as hash key; variable's type (Int or Float) as hash value.
```c
typedef struct{
    char key[65];
    DataType type;//value
}HashNode;
typedef struct{
    int size;
    HashNode* *storage;
}HashMap;
```
1. **Initialize** - Apart from the general case of hash initilization, each record node has been allocated space in order to store the default type - `Notype`.
```c
/* create and initialize hash map */
HashMap* InitializeMap(int size)
{
    /* create */
    HashMap *map = calloc(1, sizeof(HashMap));
    map->size = size;
    map->storage = calloc(size, sizeof(HashNode*));

    /* initialize */
    int i;
    for(i = 0; i < size; i++){
        map->storage[i] = calloc(1, sizeof(HashNode));//every record has a "Notype" value in predefined.
        map->storage[i]->type = Notype;//map->storage[i] is a pointer to the HashNode
    }
    return map;
}
```
2. **Create** - The function `add_table` is similiar to hash set. I renamed it to `add_map` in the modified version. It's normal that an record node already owns a type of other variable (i.e., collision), the multiple definitions error occurs when the keys are identical (at line 8); otherwise, iterate through the map until find out a record which contains `Notype`. Then we can use this record to store a new key-value pair.
```c
/* hash_set */
void add_map( HashMap *map, char* key, DataType t )
{
    int hashIdx = hash(map, key);

    HashNode *node;
    while(map->storage[hashIdx]->type != Notype){
        if(strcmp(map->storage[hashIdx]->key, key) == 0){
            printf("Error : id %s has been declared\n", key);//error
            break;
        }else{//collision
            if(hashIdx <= map->size-2){
                hashIdx++;
            }else{
                hashIdx = 0;
            }
        }
    }
    /* we can use this new record */
    memcpy(map->storage[hashIdx]->key, key, strlen(key)+1);//store key
    map->storage[hashIdx]->type = t;//store value
}

```

---
### Algorithms

The hash function used in `add_map` at line 4 is provided:
```c
int hash(HashMap *map, char *key)
{
    unsigned long hashval = 5381;
    int c;

    while(c = *key++)
        hashval = ((hashval<<5) + hashval) + c;

    return hashval % map->size;
}
```
I adopted [`djb2`](http://www.cse.yorku.ca/~oz/hash.html), one of the best hash algorithm for string.


---

3. **Lookup** - The function `lookup_table` is similiar to hash get. I renamed it to `lookup_map` in the modified version. Compare with the **Create** section, if the hashing result has no type, then an undeclared error happens. Otherwise, iterate through the map until find out a record which has an identical key with the searched identifier. 

```c
/* hash_get */
DataType lookup_map( HashMap *map, char *key )
{
    int hashIdx = hash(map, key);
    
    HashNode *node;
    while(map->storage[hashIdx]->type != Notype){
        node = map->storage[hashIdx];//has node
        if(strcmp(node->key, key)==0){
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
    return Notype;
}
```
## Task 3 : Constant Folding Optimization
“Constant folding” is a simple optimization, which evaluates constant expressions at compile time.

For example,
the following expression
`a = 10 + 20 - 5 + b`
could be turned into
`a = 25 + b`

```
You are NOT required to exploit the constant folding opportunities in the following expression:
a – 100 – 50 + 6

This is because the order of evaluation for the above expression is actually 
(((a-100) – 50) + 6)
```

---
I made this function in the phase of **semantic analysis (語意分析)**, which does the **type checking** on a parse tree (a parse tree is built after the parsing phase). The semantic analysis is to make sure a meaningful statement.

<img src="https://i.imgur.com/vRvfPGq.png" width="280">

The semantic analysis is done by the top down approach in the AC language, which recursively calls the function -  *`checkexpression`* from the root node of the parse tree. Thus, the type ( Int or Float ) is determined from the leaves.

a.<img src="https://i.imgur.com/Khvwxl1.png" height="200">&nbsp;&nbsp;b. <img src="https://i.imgur.com/HJTCjJ5.png" height="200">&nbsp;&nbsp;c.<img src="https://i.imgur.com/IKt0bSc.png" height="200">

d. <img src="https://i.imgur.com/tXaMLmF.png" height="237">&nbsp;&nbsp;e. <img src="https://i.imgur.com/4bdMrZe.png" height="237">&nbsp;&nbsp;f. <img src="https://i.imgur.com/YE3sXrG.png" height="237">

g. <img src="https://i.imgur.com/RyjYTUY.png" height="237">



### Data Structures and Functions

I modified the code in the function - `checkexpression`. 
```c
void mycheckexpression( Expression * expr, HashMap *map )
{
    char str[65];
    if(expr->leftOperand == NULL && expr->rightOperand == NULL){
        switch(expr->v.type){
            case Identifier:
                memcpy(str, expr->v.val.id, strlen(expr->v.val.id)+1);
                expr->type = lookup_map(map, str);
                break;
            case IntConst:
                expr->type = Int;
                break;
            case FloatConst:
                expr->type = Float;
                break;
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
         
        /**************original code**************/
//      convertType(left, type);//left->type = type;
//      convertType(right, type);//right->type = type;
        /*****************************************/
         
        /***Constant Folding Optimization start!**/
        bool lFlag = false, rFlag = false;
        lFlag = isConvertType(left, type);
        rFlag = isConvertType(right, type);
        
        /*if there is any child convert its type*/
        if(lFlag==true || rFlag==true){
            /*if both childs have numeric value*/
            if(lFlag && left->leftOperand->v.type == IntConst && right->v.type == FloatConst){
                /*calculate the result based on the operator*/
                calculate_op(expr, lFlag, rFlag);
                /*change the function of the node from operator to constant*/
                expr->v.type = FloatConst;
                /*constant node is always a leaf node*/
                expr->leftOperand = NULL;
                expr->rightOperand = NULL;
            }
            if(rFlag && right->leftOperand->v.type == IntConst && left->v.type == FloatConst){
                calculate_op(expr, lFlag, rFlag);
                expr->v.type = FloatConst;
                expr->leftOperand = NULL;
                expr->rightOperand = NULL;

            }
        }else{/*if there are no childs
               *  convert its type, check
               *  whether they are both
               *  constants (i.e., no one
               *  else is identifier)*/
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
        /***Constant Folding Optimization end!***/
    }
}
```
With constant folding optimization, the next of figure (d) would be:

<img src="https://i.imgur.com/sl7NiYG.png" height="200">
; thus, fewer instructions would be generated.
