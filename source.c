#include<stdio.h>
#include<stdlib.h>
#include<string.h>



typedef struct xmlNode{

  char elementName[50];
  char content[250];
  int attributeCounter;
  int childCounter;
  struct xmlNode *parentNode;
  struct xmlAttribute *attributes[20];
  struct xmlNode *childNodes[20];

}xmlNode;

typedef struct xmlAttribute{

  char keyword[100];
  char value[500];

}xmlAttribute;

///STACK OPERATIONS
xmlNode *stack[1000] = {NULL};
int stackTop = -1;

void stackPush(xmlNode *element){

  stackTop++;
  stack[stackTop] = (xmlNode*)malloc(sizeof(xmlNode));
  stack[stackTop] = element;

}

void stackPop(){

  if(stackTop > -1)
    stack[stackTop--] = NULL;

}

xmlNode* stackPeek(){

  if(stackTop > -1){
    return stack[stackTop];
  }

  return NULL;
}

int stackIsEmpty(){

  if(stackTop == -1)
    return 1;
  else
    return 0;
}

////STACK OPERATIONS

////XML PARSING OPERATIONS
void readXmlFile(FILE *fp, char* file){

  int i = 0;
  char ch;
  while((ch = fgetc(fp)) != EOF){
    file[i++] = ch;
  }
  file[i] = '\0';
}

void getFirstXmlElement(char* file, char* element){

  int i = 0;
  int j = 0;
  while(file[i++] != '<' && file[i] != '\0');
  int startIndex = i;
  while(file[i] != '>')
    element[j++] = file[i++];
  element[j] = '\0';

  while(file[++j] != '<' && file[j] != '\0');
  char temp[1000];
  int k = 0;
  for(k = 0; j < strlen(file);j++,k++){
    temp[k] = file[j];
  }
  temp[k] = '\0';
  strcpy(file, temp);

}

void checkFirstXmlElement(char* file, char* element){

  int i = 0;
  int j = 0;

  while(file[i++] != '<' && file[i] != '\0');
  int startIndex = i;
  while(file[i] != '>')
    element[j++] = file[i++];
  element[j] = '\0';
  char *tag = strtok(element, " ");
  strcpy(element, tag);
}

void getLastXmlElement(char* file, char* element){

  int i = strlen(file) - 1;
  int j = 0;
  while(file[i] != '/') i--;
  int lastIndex = i-2;
  while(file[i] != '>')
    element[j++] = file[i++];
  element[j] = '\0';

  while(file[--lastIndex] != '>' && lastIndex >= 0);
  file[lastIndex + 1] = '\0';

}

void checkLastXmlElement(char *file, char* element){

  int i = strlen(file) - 1;
  int j = 0;
  while(file[i] != '/') i--;
  int lastIndex = i-2;
  while(file[i] != '>')
    element[j++] = file[i++];
  element[j] = '\0';
}

void parseXmlAttribute(char *keyValue, xmlAttribute *attribute){

  char *keyWord = strtok(keyValue, "=");
  char *value = strtok(NULL, "=");
  char tempValue[500];
  strcpy(attribute->keyword, keyWord);
  strcpy(tempValue, value);
  tempValue[strlen(tempValue) - 1] = '\0';
  int i = 0;
  int k = 0;
  for(i = 0; i < strlen(tempValue) ; i++){
    if(tempValue[i] != '"')
      tempValue[k++] = tempValue[i];
  }
  tempValue[k] = '\0';
  strcpy(attribute->value, tempValue);
}

void parseXmlElement(char *elementBody, xmlAttribute *attributes[], char *elementTag){

  strcpy(elementTag, strtok(elementBody, " "));
  int i = 0;
  char **keyValues = (char**)malloc(sizeof(char*)*20);
  char *keyValue;
  while((keyValue = strtok(NULL, " ")) != NULL){

    keyValues[i] = (char*)malloc(sizeof(char) * 50);
    strcpy(keyValues[i++], keyValue);
  }
  i = 0;
  while(keyValues[i] != NULL){
    attributes[i] = (xmlAttribute*)malloc(sizeof(xmlAttribute));
    parseXmlAttribute(keyValues[i], attributes[i]);
    i++;
  }

}

void createRootXmlNode(char *file, xmlNode *node){

  char elementBody[500] = {'\0'};
  char closingTag[100]={'\0'};

  getFirstXmlElement(file, elementBody);
  parseXmlElement(elementBody, node->attributes, node->elementName);
  checkLastXmlElement(file, closingTag);
  strcpy(closingTag, closingTag + 1);
  if(strcmp(node->elementName, closingTag) == 0){
    node->parentNode = NULL;
    node->childCounter = 0;
  }
  else{
    printf("Wrong root element\n");
    printf("Opening tag: %s\nClosing tag: %s\n", node->elementName, closingTag);
    exit(1);
  }
}

void getXmlElementContent(char *file, char *content, char *elementName){

  int startingPoint = 0;
  while(file[startingPoint++] != '>');
  int endingPoint = startingPoint;
  while(file[endingPoint++] != '<');

  int i = 0;
  for(i = 0;startingPoint < endingPoint-1; startingPoint++){
    if(file[startingPoint] != '\n' && file[startingPoint] != ' ')
      content[i++] = file[startingPoint];
  }
  content[i] = '\0';
}

void parseXml(char *fileName, xmlNode *root){

  char file[5000];
  FILE *fp;
  fp = fopen(fileName, "r");
  if( fp != NULL){
    readXmlFile(fp, file);
  }else{
    printf("File can not be opened.\n");
    exit(1);
  }

  char firstElement[500];
  char lastElement[500];
  char content[500] = {'\0'};
  checkFirstXmlElement(file, firstElement);

  if(strcmp(firstElement, "?xml") == 0) getFirstXmlElement(file, firstElement);
  createRootXmlNode(file, root);
  stackPush(root);

  while(!stackIsEmpty()){


    checkFirstXmlElement(file, firstElement);
    if(firstElement[0] == '/'){
      stackPop();
      getFirstXmlElement(file, firstElement);
    }
    else{
      xmlNode *node = (xmlNode*)malloc(sizeof(xmlNode));
      checkFirstXmlElement(file,firstElement);
      getXmlElementContent(file, node->content, firstElement);
      getFirstXmlElement(file, firstElement);
      int flag = 0;
      if(firstElement[strlen(firstElement) - 1] == '/') flag = 1;
      parseXmlElement(firstElement, node->attributes, node->elementName);
      node->childCounter = 0;
      node->parentNode = (xmlNode*)malloc(sizeof(xmlNode));
      node->parentNode = stackPeek();
      node->parentNode->childNodes[node->parentNode->childCounter] = (xmlNode*)malloc(sizeof(xmlNode));
      node->parentNode->childNodes[node->parentNode->childCounter] = node;
      node->parentNode->childCounter += 1;
      stackPush(node);
      if(flag) stackPop();


    }
  }
}

void printTree(xmlNode * node){


  printf("Element Name: %s\nContent : %s\n", node->elementName, node->content);
  printf("Child Number: %d\nParentNode : %s\n", node->childCounter, node->parentNode->elementName);
  printf("Attributes: ");
  int k = 0;
  while(node->attributes[k] != NULL){
    printf("%s = %s", node->attributes[k]->keyword, node->attributes[k]->value);
    k++;
    if(node->attributes[k] != NULL) printf(", ");
  }
  puts("");
  int i = 0;
  for(i = 0 ; i < node->childCounter ; i++){
    puts("----------");
    printTree(node->childNodes[i]);
  }

}

///XML PARSING OPERATIONS

///JSON WRITING OPERATIONS

void createJsonFile(xmlNode *node, FILE *fp, int depth){

  char spaces[depth + 1];
  int i = 0;
  for(i = 0 ; i < depth ; i++) spaces[i] = '\t';
  spaces[i] = '\0';

  if(node->childCounter > 0 ){

    fprintf(fp, "%s\"%s\": {\n", spaces, node->elementName);

    if(node->attributes[0] != NULL){
      i = 0;
      while(node->attributes[i] != NULL){
        fprintf(fp, "%s\t\"-%s\": \"%s\",\n",spaces, node->attributes[i]->keyword, node->attributes[i]->value);
        i++;
      }
    }
    if(strcmp(node->content, "") != 0){
      fprintf(fp, "%s\t\"-%s\": \"%s\",\n",spaces, "#text:" , node->content);
    }
    for(i = 0 ; i < node->childCounter ; i++){
      createJsonFile(node->childNodes[i], fp, depth + 1 );
    }
    fprintf(fp, "%s},\n", spaces);
  }
  else if(node->attributes[0] != NULL){
    fprintf(fp, "%s\"%s\": {", spaces, node->elementName);
    i = 0;
    while(node->attributes[i] != NULL){

      fprintf(fp, "\"-%s\": \"%s\"",node->attributes[i]->keyword, node->attributes[i]->value);
      i++;
      if(node->attributes[i] != NULL) fprintf(fp,", ");
    }
    if(strcmp(node->content, "") != 0){
      fprintf(fp, ", \"%s\": \"%s\"", "#text:" , node->content);
    }
    fprintf(fp, "}\n");
  }
  else{

    fprintf(fp, "%s\"%s\": \"%s\",\n",spaces, node->elementName, node->content);
  }

}

void writeJson(xmlNode *root, char *fileName){

  FILE *fp;
  fp = fopen(fileName, "w");
  fprintf(fp,"{\n");
  createJsonFile(root, fp, 1);
  fprintf(fp,"}");
}


///JSON WRITING OPERATIONS



int main(){

  xmlNode *root = (xmlNode*)malloc(sizeof(xmlNode));
  parseXml("test.xml", root);
  //printTree(root);
  writeJson(root, "test.json");

  return 0;
}
