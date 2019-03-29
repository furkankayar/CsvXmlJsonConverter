#include <stdio.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <string.h>
#include <json/json.h>

const char* jsonRootName;
char** csvColumnNames;
char* csvFile;

typedef struct _csvRow{

	char **columns;
	struct _csvRow *next;

}csvRow;

static char* readFile(char* fileName){

	FILE *fp = fopen(fileName, "r");

	if( fp == NULL){
		printf("Error: %s has not been found. Please check file and try again.\n", fileName);
		exit(0);
	}
	char *file = (char*)malloc(sizeof(char) * 100000);
	char ch;
	int counter = 0;

	while((ch = getc(fp)) != EOF)	file[counter++] = ch;
	file[counter] = '\0';

	return file;

}

static char** csvGetRow(){

	int i = 0;
	char *row = (char*)malloc(sizeof(char) * 5000);
	char **rowColumns = (char**)malloc(sizeof(char*) * 200);
	if(strcmp(csvFile, "") != 0){
		while(csvFile[i] != '\n'){
			row[i] = csvFile[i];
			i++;
		}
		row[i] = '\0';
		csvFile = csvFile + i + 1;

		char * ptr = strtok(row, ",");
		int k = 0;
		while(ptr != NULL){
			rowColumns[k] = (char*)malloc(sizeof(char) * 200);
			strcpy(rowColumns[k], ptr);
			ptr = strtok(NULL, ",");
			k++;
		}
	}

	return rowColumns;
}

static csvRow* csvParseFile(){

	char **columns = csvGetRow();
	csvRow * csv_root = (csvRow*)malloc(sizeof(csvRow));
	csvRow * csv_temp = csv_root;
	while(columns[0] != NULL){

		csv_root->columns = columns;
		columns = csvGetRow();
		if(columns[0] != NULL) csv_root->next = (csvRow*)malloc(sizeof(csvRow));
		csv_root = csv_root->next;
	}

	return csv_temp;
}

static char* jsonGetRootName(json_object *jsonObj){

	const char *jsonAsStr = json_object_to_json_string(jsonObj);
	int i = 0;
	char *rootName = (char*)malloc(sizeof(char) * 250);
	while(jsonAsStr[i++] != '"');
	int k = 0;
	while(jsonAsStr[i] != '"'){
		rootName[k++] = jsonAsStr[i++];
	}
	return rootName;
}

static int getXmlElementChildNumber(xmlNode *node){
	int i = 0;
	node = node->children;
	while(node != NULL){
		if(strcmp(node->name, "text") != 0)
			i++;
		node = node->next;
	}
	return i;
}

static void xmlToJson(xmlNode * xml_node, json_object * jsonObj){ // PERFECTION

	xmlNode *cur_node = NULL;
	char lastName[250] = {'\0'};
	json_object *newObj = NULL;
	json_object *jsonArr = NULL;
  int isArrayCreated = 0;

	for (cur_node = xml_node; cur_node; cur_node = cur_node->next) {

		 if(cur_node->type == XML_ELEMENT_NODE){

			 if(getXmlElementChildNumber(cur_node) == 0){
				 char *content = cur_node->children->content;
				 int i = 0;
				 for(i = 0 ; i < strlen(content); i++){
					 if(content[i] == '\n') content[i] = ' ';
				 }
				 newObj = json_object_new_string(cur_node->children->content);
			 }
			 else{
				 newObj = json_object_new_object();
				 if(cur_node->properties != NULL){
					 newObj = json_object_new_object();
					 xmlAttr *attribute = cur_node->properties;
					 if(attribute != NULL){
							do{
								char *key = (char*)malloc(sizeof(char) * 100);
								char *value = (char*)malloc(sizeof(char) * 100);
								strcpy(key, attribute->name);
								char *keyword = (char*)malloc(sizeof(char) * 100);
								keyword[0] = '-';
								keyword[1] = '\0';
								strcat(keyword, key);
								strcpy(value, attribute->children->content);
								json_object *attrValue = json_object_new_string(value);
								json_object_object_add(newObj, keyword, attrValue);
								attribute = attribute->next;
							}while(attribute != NULL);
					 }
				 }
			 }

			 xmlToJson(cur_node->children, newObj); // RECURSION
			 xmlNode *nextNode = cur_node->next;
			 while(nextNode && strcmp(nextNode->name, "text") == 0) nextNode = nextNode->next;
			 if((nextNode && strcmp(cur_node->name, nextNode->name) == 0) || strcmp(cur_node->name, lastName) == 0){
         if(isArrayCreated == 0){
           jsonArr = json_object_new_array();
           json_object_object_add(jsonObj, cur_node->name, jsonArr);
           json_object_array_add(jsonArr, newObj);
           strcpy(lastName, cur_node->name);
           isArrayCreated = 1;
         }
         else{
          json_object_array_add(jsonArr, newObj);
          strcpy(lastName, cur_node->name);
         }
			 }else{
         isArrayCreated = 0;
         json_object_object_add(jsonObj, cur_node->name, newObj);
       }
		 }
	}
}

static void jsonToXml(json_object * jobj, xmlNodePtr xmlNode){

	json_object *object;
	int arrLen;
	enum json_type type;
	json_object_object_foreach(jobj, key, val){

		type = json_object_get_type(val);

		switch(type){

			case json_type_object:

				object = val;
				printf("%s\n", key);
				if(strcmp(key, jsonRootName) != 0){
					xmlNodePtr newNode = xmlNewNode(NULL, BAD_CAST key);
					xmlAddChildList(xmlNode, newNode);
					jsonToXml(object, newNode);
				}
				else{
			 		jsonToXml(object, xmlNode);
				}

			break;
			case json_type_array:

			  arrLen = json_object_array_length(val);
				int i = 0;
				for(i = 0 ; i < arrLen ; i++){
					object = json_object_array_get_idx(val, i);
					xmlNodePtr newNode = xmlNewNode(NULL, BAD_CAST key);
					xmlAddChildList(xmlNode, newNode);
					jsonToXml(object, newNode);
				}

			break;
			default: // FOR ANY JSON TYPE

				xmlNewChild(xmlNode, NULL, BAD_CAST key, BAD_CAST json_object_get_string(val));

			break;
		}
	}
}

static void csvToJson(csvRow* csvObj, json_object *jobj){

	jobj = json_object_new_object();

  int rowCounter = 1;
	while(csvObj != NULL){
		json_object *row = json_object_new_object();
		int i = 0;
		while(csvObj->columns[i] != NULL){
			json_object * column = json_object_new_string(csvObj->columns[i]);
			json_object_object_add(row, csvColumnNames[i], column);
			i++;
		}
		char str[20];
		sprintf(str, "row%d", rowCounter++);
		json_object_object_add(jobj, str, row);
		csvObj = csvObj->next;
	}

	printf("%s\n", json_object_to_json_string(jobj));
}

int main(){



 	/*xmlDocPtr doc = NULL;
	xmlNode *root_element = NULL;
  xmlNode *next_element = NULL;
	const char *fileName = "lab5.xml";
	doc = xmlParseFile(fileName);

	if (doc == NULL){
		printf("Error: %s has not been found. Please check the file and try again.\n", fileName);
		exit(0);
	}


  root_element = xmlDocGetRootElement(doc);
	json_object *json_root = json_object_new_object();
	xmlToJson(root_element, json_root);
	printf("%s\n", json_object_to_json_string(json_root));
	json_object_to_file("test.json", json_root);*/




	json_object * jobj = json_tokener_parse(readFile("test.json"));
	xmlDocPtr doc = xmlNewDoc("1.0");
	jsonRootName = jsonGetRootName(jobj);
	xmlNodePtr root_node = xmlNewNode(NULL, jsonRootName);
	jsonToXml(jobj, root_node);
	xmlDocSetRootElement(doc, root_node);
	xmlSaveFormatFileEnc("write.xml", doc, "UTF-8", 0);

	/*csvFile = readFile("inputForDebian.csv");
	csvColumnNames = csvGetRow();
	csvRow* csv_root;
	csv_root = csvParseFile();

	json_object *json_root;
	csvToJson(csv_root, json_root);*/

	return 0;
}
