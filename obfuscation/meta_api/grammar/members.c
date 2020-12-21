#include <string>

using namespace std;

OP_Datastructure* currentStructure;
OP_Transformation* currentTransformation;
OP_Rule* currentRule;
OP_Function* currentFunction;

typedef unsigned char * pANTLR3UINT8;

void initDatastructure() {
	currentStructure = new OP_Datastructure();

	currentStructure->inits.clear();
	currentStructure->predicates.clear();
	currentStructure->transformations.clear();
}

void initTransformation() {
	currentTransformation = new OP_Transformation();

	currentTransformation->function = new OP_Function();
	currentTransformation->rules.clear();
}

void setTransformationName( pANTLR3UINT8 name ) {
	if( currentTransformation == NULL)
		initTransformation();

	currentTransformation->function->name = string(reinterpret_cast<char*>(name));
}

void addTransformationToStructure() {
	if( currentStructure == NULL || currentTransformation == NULL)
		return;

	currentStructure->transformations.push_back(currentTransformation);
	currentTransformation = NULL;
}

void addArgDataTupleToTransformation( const char* argument ){
	OP_Arg_data_tuple *a = new OP_Arg_data_tuple();
	currentTransformation->function->arguments.push_back(a);

	a->argument = string(argument);
}

void splitArgumentListTransformation( pANTLR3UINT8 argumentList ){
	char * pch;
	//printf ("Splitting string \"\%s\" into tokens:\n",argumentList);
	pch = strtok (reinterpret_cast<char*>(argumentList)," ,");
	while (pch != NULL)
	{
		//printf ("\%s\n",pch);
		//addArgDataTupleToTransformation( pch );
		pch = strtok (NULL, " ,");
	}
}

void addCondition( const char* var1, const char* operation, char* condition) {
	int length;
	char* pch;
	char* end;

	OP_Condition *c = new OP_Condition();
	currentRule->conditions.push_back(c);

	c->var1 = string(var1);

	if( !strcmp(operation,"==") ){
		c->relation = EQUAL;
	} else if( !strcmp(operation,">") ){
		c->relation = GREATER_THAN;
	} else if( !strcmp(operation,"<") ){
		c->relation = SMALLER_THAN;
	} else if( !strcmp(operation,">=") ){
		c->relation = GREATER_EQUAL;
	} else if( !strcmp(operation,"<=") ){
		c->relation = SMALLER_EQUAL;
	} else if( !strcmp(operation,"!=") ){
		c->relation = NOT_EQUAL;
	}

	pch = strtok (condition," ");
	pch = strtok (NULL," ");
	pch = strtok (NULL," ");

	if( isalpha(pch[0]) ){
		c->var2 = string(pch);
		c->input = 0;
	} else {
		c->var2 = "";
		c->input = strtol (pch,&end,10);
	}
	c->isAll = false;
	/*currentRule->numberConditions++;
	currentRule->conditions = realloc( currentRule->conditions, sizeof(struct OP_Condition*) * currentRule->numberConditions);

	currentRule->conditions.back() = toAdd;*/
}

void addAllCondition(){
	//printf("ADD ALL CONDITION");
	OP_Condition *c = new OP_Condition();
	currentRule->conditions.push_back(c);

	c->var1 = "";
	c->var2 = "";
	c->isAll = true;
}

void addRule( pANTLR3UINT8 var1, pANTLR3UINT8 operation, pANTLR3UINT8 condition, int all) {
	//printf("Allocate rule\n");
	currentRule = new OP_Rule();
	currentRule->conditions.clear();
	currentRule->effects.clear();

	if( all == 0){
		addCondition(reinterpret_cast<const char*>(var1), reinterpret_cast<const char*>(operation), reinterpret_cast<char*>(condition) );
	} else {
		addAllCondition();
	}

	currentTransformation->rules.push_back(currentRule);
}

void addEffect(pANTLR3UINT8 var, pANTLR3UINT8 effect_) {
	OP_Effect_tuple *e = new OP_Effect_tuple();
	currentRule->effects.push_back(e);

	e->namePredicate = string(reinterpret_cast<char*>(var));

	const char *effect = reinterpret_cast<const char*>(effect_);
	if( effect[0] == 'T' ){
		e->effect = E_TRUE;
	} else if( effect[0] == 'F' ){
		e->effect = E_FALSE;
	} else if( effect[0] == '?' ){
		e->effect = E_UNKNOWN;
	} else if( effect[0] == 'U' ){
		e->effect = E_UNCHANGED;
	}
}
