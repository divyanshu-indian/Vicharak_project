#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#define MAX_TOKEN_LENGTH 100

// Token types enumeration
typedef enum {
    TOK_INT, TOK_ID, TOK_NUM, TOK_ASSIGN,
    TOK_PLUS, TOK_MINUS, TOK_IF, TOK_EQ, TOK_LBRACE, TOK_RBRACE,
    TOK_SEMICOLON, TOK_END
} TokenType;

// Token structure
typedef struct {
    TokenType type;
    char value[MAX_TOKEN_LENGTH];
} Token;

// AST node types enumeration
typedef enum {
    NODE_DECL, NODE_ASSIGNMENT, NODE_EXPR, NODE_COND, NODE_VAL, NODE_VAR
} NodeType;

// AST node structure
typedef struct ASTNode {
    NodeType type;
    char data[MAX_TOKEN_LENGTH];
    struct ASTNode *left;
    struct ASTNode *right;
} ASTNode;

// Function to tokenize the input
void getNextToken(FILE *source, Token *token) {
    int ch;
    while ((ch = fgetc(source)) != EOF) {
        if (isspace(ch)) continue;

        if (isalpha(ch)) {
            int len = 0;
            token->value[len++] = ch;
            while (isalnum(ch = fgetc(source))) {
                if (len < MAX_TOKEN_LENGTH - 1) token->value[len++] = ch;
            }
            ungetc(ch, source);
            token->value[len] = '\0';

            if (strcmp(token->value, "int") == 0) token->type = TOK_INT;
            else if (strcmp(token->value, "if") == 0) token->type = TOK_IF;
            else token->type = TOK_ID;
            return;
        }

        if (isdigit(ch)) {
            int len = 0;
            token->value[len++] = ch;
            while (isdigit(ch = fgetc(source))) {
                if (len < MAX_TOKEN_LENGTH - 1) token->value[len++] = ch;
            }
            ungetc(ch, source);
            token->value[len] = '\0';
            token->type = TOK_NUM;
            return;
        }

        switch (ch) {
            case '=': token->type = TOK_ASSIGN; token->value[0] = '='; token->value[1] = '\0'; return;
            case '+': token->type = TOK_PLUS; token->value[0] = '+'; token->value[1] = '\0'; return;
            case '-': token->type = TOK_MINUS; token->value[0] = '-'; token->value[1] = '\0'; return;
            case '{': token->type = TOK_LBRACE; token->value[0] = '{'; token->value[1] = '\0'; return;
            case '}': token->type = TOK_RBRACE; token->value[0] = '}'; token->value[1] = '\0'; return;
            case ';': token->type = TOK_SEMICOLON; token->value[0] = ';'; token->value[1] = '\0'; return;
        }
    }

    token->type = TOK_END;
    token->value[0] = '\0';
}

// Function to create an AST node
ASTNode *newNode(NodeType type, const char *data) {
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    node->type = type;
    strcpy(node->data, data);
    node->left = NULL;
    node->right = NULL;
    return node;
}

// Function prototypes
ASTNode *parseExpression(Token *current, FILE *source);
ASTNode *parseStatement(Token *current, FILE *source);
ASTNode *parseConditional(Token *current, FILE *source);
ASTNode *parseAssignment(Token *current, FILE *source);
ASTNode *parseDeclaration(Token *current, FILE *source);

// Function to parse a variable declaration
ASTNode *parseDeclaration(Token *current, FILE *source) {
    getNextToken(source, current); // Expect identifier
    if (current->type != TOK_ID) {
        printf("Syntax Error: Expected identifier\n");
        exit(1);
    }

    ASTNode *node = newNode(NODE_DECL, current->value);
    getNextToken(source, current); // Expecting ';'
    if (current->type != TOK_SEMICOLON) {
        printf("Syntax Error: Expected ';'\n");
        exit(1);
    }

    return node;
}

// Function to parse an assignment
ASTNode *parseAssignment(Token *current, FILE *source) {
    if (current->type != TOK_ID) {
        printf("Syntax Error: Expected identifier\n");
        exit(1);
    }

    ASTNode *node = newNode(NODE_ASSIGNMENT, current->value);
    getNextToken(source, current); // Expect '='
    if (current->type != TOK_ASSIGN) {
        printf("Syntax Error: Expected '='\n");
        exit(1);
    }

    node->right = parseExpression(current, source);

    getNextToken(source, current); // Expecting ';'
    if (current->type != TOK_SEMICOLON) {
        printf("Syntax Error: Expected ';'\n");
        exit(1);
    }

    return node;
}

// Function to parse an expression
ASTNode *parseExpression(Token *current, FILE *source) {
    getNextToken(source, current);

    if (current->type == TOK_NUM || current->type == TOK_ID) {
        return newNode(NODE_VAL, current->value);
    }

    return NULL;
}

// Function to parse an if-statement
ASTNode *parseConditional(Token *current, FILE *source) {
    // Expect '(' after 'if'
    getNextToken(source, current);
    if (current->type != TOK_LBRACE) {
        printf("Syntax Error: Expected '('\n");
        exit(1);
    }

    // Parse the condition expression
    ASTNode *condition = parseExpression(current, source);

    // Expect ')' after the condition
    getNextToken(source, current);
    if (current->type != TOK_RBRACE) {
        printf("Syntax Error: Expected ')'\n");
        exit(1);
    }

    // Expect '{' to start the block
    getNextToken(source, current);
    if (current->type != TOK_LBRACE) {
        printf("Syntax Error: Expected '{'\n");
        exit(1);
    }

    // Parse the block (the statements inside the if block)
    ASTNode *body = parseStatement(current, source);

    // Expecting '}' to end the block
    getNextToken(source, current);
    if (current->type != TOK_RBRACE) {
        printf("Syntax Error: Expected '}'\n");
        exit(1);
    }

    // Create a conditional node with the condition and body
    ASTNode *ifNode = newNode(NODE_COND, "");
    ifNode->left = condition;
    ifNode->right = body;

    return ifNode;
}

// Function to parse a statement
ASTNode *parseStatement(Token *current, FILE *source) {
    getNextToken(source, current);

    if (current->type == TOK_INT) {
        return parseDeclaration(current, source);
    } else if (current->type == TOK_ID) {
        return parseAssignment(current, source);
    } else if (current->type == TOK_IF) {
        return parseConditional(current, source);
    } else if (current->type == TOK_LBRACE) {
        // Handle block of statements inside { }
        ASTNode *blockNode = NULL;
        ASTNode *lastNode = NULL;
        while (1) {
            ASTNode *statement = parseStatement(current, source);
            if (statement == NULL || current->type == TOK_RBRACE) break;
            if (blockNode == NULL) {
                blockNode = statement;
            } else {
                lastNode->right = statement;
            }
            lastNode = statement;
        }
        return blockNode;
    }

    return NULL;
}

// Function to generate assembly code from the AST
void generateAssembly(ASTNode *node) {
    if (node == NULL) return;

    switch (node->type) {
        case NODE_DECL:
            printf("VAR %s\n", node->data);
            break;

        case NODE_ASSIGNMENT:
            generateAssembly(node->right);
            printf("STORE %s\n", node->data);
            break;

        case NODE_VAL:
            printf("LOAD %s\n", node->data);
            break;

        case NODE_COND:
            generateAssembly(node->left); // Condition
            printf("JZ else_label\n");
            generateAssembly(node->right); // If body
            printf("else_label:\n");
            break;

        default:
            printf("Unknown node type\n");
            break;
    }
}

// Main function
int main() {
    FILE *source = fopen("input", "r");
    if (!source) {
        perror("Error opening file");
        return 1;
    }

    Token currentToken;

    while (1) {
        ASTNode *statement = parseStatement(&currentToken, source);
        if (statement == NULL) break;
        generateAssembly(statement);
        free(statement);
    }

    fclose(source);
    return 0;
}
