#include <stdio.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlschemastypes.h>
#include <string.h>
#include <json/json.h>

const char* jsonRootName;
char** csvColumnNames;
int csvColumnNumber = 0;
char* csvFile;
int columnCount = 0;

typedef struct _csvRow{

	char **columns;
	struct _csvRow *next;

}csvRow;

static char* getFileExtension(char *fileName){

	char *extension = (char*)malloc(sizeof(char) * 50);
	int i = strlen(fileName) - 1;
	while(fileName[i - 1] != '.') i--;
	int k = 0;
	while(i < strlen(fileName)){
		extension[k++] = fileName[i++];
	}

	return extension;
}

static int contains(char **strArr, char *str){

	int i = 0;
	while(strArr[i] != NULL){
		if(strcmp(strArr[i], str) == 0)
			return 1;
		i++;
	}
	return 0;
}

static char* replaceAll(char* str, char oldCh, char newCh){
	char *copy = (char*)malloc(sizeof(char) * 500);
	strcpy(copy, str);
	int i = 0;
	for(i = 0 ; i < strlen(copy) ; i++) if(copy[i] == oldCh) copy[i] = newCh;
	return copy;
}

static char* trim(const char* str, int option){

	char *copy = (char*)malloc(sizeof(char) * 500);
	strcpy(copy, str);
	while(copy[0] == ' ' || copy[0] == '"') copy = copy + 1;
	while(copy[strlen(copy) - 1] == ' ' || copy[strlen(copy) - 1] == '"') copy[strlen(copy) - 1] = '\0';

	int flag = 0;
	int k = 0;
	for(k = 0 ; k < strlen(copy); k++){
		if(copy[k] == ',') flag = 1;
	}
	if(flag && option == 1){
		k = strlen(copy) + 1;
		copy[k] = '"';
		int i = 0;
		for(i = k - 1 ; i >= 0 ; i--){
			copy[i] = copy[i - 1];
		}
		copy[0] = '"';
	}

	return copy;
}

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

static char* readJsonFile(char* fileName){

	char* file = readFile(fileName);
	if(file[0] == '['){
		char *root = (char*)malloc(sizeof(char) * 100000);
		strcpy(root, file);
		strcpy(file, "{\"root\": ");
		strcat(file, root);
		file[strlen(file)] = '}';
	}

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

		if(row[i - 1] == 13) row[i - 1] = '\0';
		else row[i] = '\0';

		csvFile = csvFile + i + 1;

		char *column = (char*)malloc(sizeof(char) * 500);
		int k = 0;
		int t = 0;
		int z = 0;
		for(t = 0 ; t < strlen(row) ; t++){
			while(row[t] != ',' && t < strlen(row)){
				column[z++] = row[t++];
			}
			column[z] = '\0';
			rowColumns[k] = (char*)malloc(sizeof(char) * 200);
			strcpy(rowColumns[k], column);
			z = 0;
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

static void csvCreateColumnNamesByJson(json_object *jsonObj){

	int arrLen = 0;
	json_object *object;
	char * str;
	enum json_type type;
	json_object_object_foreach(jsonObj, key, val){

		type = json_object_get_type(val);
		switch(type){

			case json_type_object:

				csvCreateColumnNamesByJson(val);

			break;
			case json_type_array:

				arrLen = json_object_array_length(val);
				int i = 0;
				for(i = 0 ; i < arrLen ; i++){
					object = json_object_array_get_idx(val, i);
					csvCreateColumnNamesByJson(object);
				}

			break;
			default:

				str = (char*)malloc(sizeof(char) * 200);
				strcpy(str, key);
				if(contains(csvColumnNames, str) != 1){
					csvColumnNames[csvColumnNumber] = (char*)malloc(sizeof(char) * 200);
					strcpy(csvColumnNames[csvColumnNumber], key);
					csvColumnNumber++;
				}
			break;
		}
	}
}

static void csvCreateColumnNamesByXml(xmlNode *xml_node){

	xmlAttr *attribute = NULL;
  xmlNode *cur_node = NULL;
	char* str = NULL;

  for (cur_node = xml_node; cur_node; cur_node = cur_node->next) {
    if (cur_node->type == XML_ELEMENT_NODE) {
    	attribute = cur_node->properties;
    	while(attribute){
				str = (char*)malloc(sizeof(char) * 200);
				strcpy(str, attribute->name);
				if(contains(csvColumnNames, str) != 1){
					csvColumnNames[csvColumnNumber] = (char*)malloc(sizeof(char) * 200);
					strcpy(csvColumnNames[csvColumnNumber], str);
					csvColumnNumber++;
				}
        attribute = attribute->next;
      }
    }
    csvCreateColumnNamesByXml(cur_node->children);
  }
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
				if(cur_node->children !=NULL){
					char *content = cur_node->children->content;
					int i = 0;
					for(i = 0 ; i < strlen(content); i++)
						if(content[i] == '\n') content[i] = ' ';

					if(cur_node->properties != NULL){
						newObj = json_object_new_object();
						xmlAttr *attribute = cur_node->properties;
						if(attribute != NULL){
							do{
								json_object_object_add(newObj, attribute->name, json_object_new_string(attribute->children->content));
								attribute = attribute->next;
							}while(attribute != NULL);
						}
						json_object_object_add(newObj, "#text", json_object_new_string(cur_node->children->content));
				 	}
					else
						newObj = json_object_new_string(cur_node->children->content);
				}
				else{
					if(cur_node->properties != NULL){
						newObj = json_object_new_object();
						xmlAttr *attribute = cur_node->properties;
						if(attribute != NULL){
							do{
								json_object_object_add(newObj, attribute->name, json_object_new_string(attribute->children->content));
								attribute = attribute->next;
							}while(attribute != NULL);
						}
				 	}
					else
						newObj = json_object_new_string("");
				}
			}
			else{
				newObj = json_object_new_object();
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

static void jsonToXml(json_object * jobj, xmlNodePtr xmlNode){ // PERFECTION

	json_object *object;
	int arrLen;
	enum json_type type;
	json_object_object_foreach(jobj, key, val){

		type = json_object_get_type(val);

		switch(type){

			case json_type_object:

				object = val;
				if(strcmp(key, jsonRootName) != 0){
					xmlNodePtr newNode = xmlNewNode(NULL, BAD_CAST replaceAll(key, ' ', '_'));
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
					xmlNodePtr newNode = xmlNewNode(NULL, BAD_CAST replaceAll(key, ' ', '_'));
					xmlAddChildList(xmlNode, newNode);
					jsonToXml(object, newNode);
				}

			break;
			default: // FOR ANY JSON TYPE

				xmlNewChild(xmlNode, NULL, BAD_CAST replaceAll(key, ' ', '_'), BAD_CAST trim(json_object_get_string(val),0));

			break;
		}
	}
}

static void csvToJson(csvRow* csvObj, json_object *jobj){ //COMPLETED

	while(csvObj != NULL){
		json_object *row = json_object_new_object();
		int i = 0;
		while(csvObj->columns[i] != NULL){
			json_object * column = json_object_new_string(csvObj->columns[i]);
			json_object_object_add(row, csvColumnNames[i], column);
			i++;
		}
		json_object_array_add(jobj,row);
		csvObj = csvObj->next;
	}
}

static void csvToXml(csvRow* csvObj, xmlNodePtr xmlNode){ // COMPLETED

	while(csvObj != NULL){

		xmlNodePtr newNode = xmlNewNode(NULL, BAD_CAST "row");
		int i = 0;
		while(csvObj->columns[i] != NULL){

			xmlNewProp(newNode, BAD_CAST replaceAll(csvColumnNames[i], ' ', '_'), BAD_CAST csvObj->columns[i]);
			//xmlNewChild(newNode, NULL, BAD_CAST replaceAll(csvColumnNames[i], ' ', '_'), BAD_CAST csvObj->columns[i]);
			i++;
		}
		xmlAddChildList(xmlNode, newNode);
		csvObj = csvObj->next;
	}
}

static void jsonToCsv(json_object *jsonObj){ //COMPLETED

	int arrLen = 0;
	json_object *object;
	enum json_type type;
	json_object_object_foreach(jsonObj, key, val){

		type = json_object_get_type(val);
		switch(type){

			case json_type_object:

				jsonToCsv(val);

			break;
			case json_type_array:

				arrLen = json_object_array_length(val);
				int i = 0;
				for(i = 0 ; i < arrLen ; i++){
					object = json_object_array_get_idx(val, i);
					jsonToCsv(object);
				}

			break;
			default:

				while(strcmp(csvColumnNames[columnCount % csvColumnNumber], key) != 0){
					strcat(csvFile, ",");
					columnCount++;
					if(columnCount % csvColumnNumber == 0){
						csvFile[strlen(csvFile) - 1] = '\0';
						strcat(csvFile, "\n");
					}
				}
				strcat(csvFile, trim(json_object_to_json_string(val),1));
				strcat(csvFile, ",");
				columnCount++;
				if(columnCount % csvColumnNumber == 0){
					csvFile[strlen(csvFile) - 1] = '\0';
					strcat(csvFile, "\n");
				}

			break;
		}
	}
}

static void xmlToCsv(xmlNode *xml_node){

	xmlAttr *attribute = NULL;
  xmlNode *cur_node = NULL;
	char* str = NULL;

  for (cur_node = xml_node; cur_node; cur_node = cur_node->next) {
    if (cur_node->type == XML_ELEMENT_NODE) {
    	attribute = cur_node->properties;
    	while(attribute){
				str = (char*)malloc(sizeof(char) * 200);
				strcpy(str, attribute->name);
				while(strcmp(csvColumnNames[columnCount % csvColumnNumber], str) != 0){
					strcat(csvFile, ",");
					columnCount++;
					if(columnCount % csvColumnNumber == 0){
						csvFile[strlen(csvFile) - 1] = '\0';
						strcat(csvFile, "\n");
					}
				}
				strcat(csvFile, trim(attribute->children->content,0));
				strcat(csvFile, ",");
				columnCount++;
				if(columnCount % csvColumnNumber == 0){
					csvFile[strlen(csvFile) - 1] = '\0';
					strcat(csvFile, "\n");
				}

        attribute = attribute->next;
      }
    }
    xmlToCsv(cur_node->children);
  }
}

static void xmlValidate(xmlDocPtr doc, xmlSchemaPtr schema){

	xmlSchemaValidCtxtPtr ctxt;
	int ret;

	ctxt = xmlSchemaNewValidCtxt(schema); //create an xml schemas validation context
  ret = xmlSchemaValidateDoc(ctxt, doc); //validate a document tree in memory
  if (ret == 0)
    printf("XML file validates.\n");
  else if (ret > 0) //positive error code number
	  printf("XML file fails to validate.\n");
  else
    printf("XML file validation generated an internal error.\n");

	xmlSchemaFreeValidCtxt(ctxt); //free the resources associated to the schema validation context
  xmlFreeDoc(doc);

}


int main(int argc, char **argv){

	char *inputFile;
	char *outputFile;
	int operation;

	if(argc < 4){
		puts("Error: More parameters are required!");
		exit(0);
	}
	else if(argc > 4){
		puts("Error: There are unnecessary parameters!");
		exit(0);
	}
	inputFile = argv[1];
	outputFile = argv[2];
	operation = argv[3][0] - 48;


	if(operation == 1){ // CSV to XML
		if(strcmp(getFileExtension(inputFile), "csv") != 0){
			puts("Error: Input file is not a CSV file.");
			exit(0);
		}
		if(strcmp(getFileExtension(outputFile), "xml") != 0){
			puts("Error: Output file is not a XML file.");
			exit(0);
		}
		csvFile = readFile(inputFile);
		csvColumnNames = csvGetRow();
		csvRow* csv_root;
		csv_root = csvParseFile();
		xmlDocPtr doc = xmlNewDoc("1.0");
		xmlNodePtr root_node = xmlNewNode(NULL, "root");
		csvToXml(csv_root, root_node);
		xmlDocSetRootElement(doc, root_node);
		xmlSaveFormatFileEnc(outputFile, doc, "UTF-8", 0);
	}
	else if(operation == 2){ //XML to CSV
		if(strcmp(getFileExtension(inputFile), "xml") != 0){
			puts("Error: Input file is not a XML file.");
			exit(0);
		}
		if(strcmp(getFileExtension(outputFile), "csv") != 0){
			puts("Error: Output file is not a CSV file.");
			exit(0);
		}
		xmlDocPtr doc = NULL;
		xmlNode *root_element = NULL;
		xmlNode *next_element = NULL;
		csvFile = (char*)malloc(sizeof(char) * 50000);
		FILE *fp = fopen(outputFile, "w");
		doc = xmlParseFile(inputFile);
		if (doc == NULL){
			printf("Error: %s has not been found. Please check the file and try again.\n", inputFile);
			exit(0);
		}
		root_element = xmlDocGetRootElement(doc);
		csvColumnNames = (char**)malloc(sizeof(char*) * 200);
		csvCreateColumnNamesByXml(root_element);
		int i = 0;
		while(i < csvColumnNumber){
			strcat(csvFile, csvColumnNames[i++]);
			if(i < csvColumnNumber)
				strcat(csvFile, ",");
			else
				strcat(csvFile, "\n");
		}
		xmlToCsv(root_element);
		csvFile[strlen(csvFile) - 1] = '\0';
		fprintf(fp,"%s", csvFile);
		fclose(fp);
	}
	else if(operation == 3){//XML to JSON
		if(strcmp(getFileExtension(inputFile), "xml") != 0){
			puts("Error: Input file is not a XML file.");
			exit(0);
		}
		if(strcmp(getFileExtension(outputFile), "json") != 0){
			puts("Error: Output file is not a JSON file.");
			exit(0);
		}
		xmlDocPtr doc = NULL;
		xmlNode *root_element = NULL;
		xmlNode *next_element = NULL;
		doc = xmlParseFile(inputFile);
		if (doc == NULL){
			printf("Error: %s has not been found. Please check the file and try again.\n", inputFile);
			exit(0);
		}
		root_element = xmlDocGetRootElement(doc);
		json_object *json_root = json_object_new_object();
		xmlToJson(root_element, json_root);
		json_object_to_file(outputFile, json_root);
	}
	else if(operation == 4){ // JSON to XML
		if(strcmp(getFileExtension(inputFile), "json") != 0){
			puts("Error: Input file is not a JSON file.");
			exit(0);
		}
		if(strcmp(getFileExtension(outputFile), "xml") != 0){
			puts("Error: Output file is not a XML file.");
			exit(0);
		}
		json_object * jobj = json_tokener_parse(readJsonFile(inputFile));
		xmlDocPtr doc = xmlNewDoc("1.0");
		jsonRootName = jsonGetRootName(jobj);
		xmlNodePtr root_node = xmlNewNode(NULL, jsonRootName);
		jsonToXml(jobj, root_node);
		xmlDocSetRootElement(doc, root_node);
		xmlSaveFormatFileEnc(outputFile, doc, "UTF-8", 0);
	}
	else if(operation == 5){// CSV to JSON
		if(strcmp(getFileExtension(inputFile), "csv") != 0){
			puts("Error: Input file is not a CSV file.");
			exit(0);
		}
		if(strcmp(getFileExtension(outputFile), "json") != 0){
			puts("Error: Output file is not a JSON file.");
			exit(0);
		}
		csvFile = readFile(inputFile);
		csvColumnNames = csvGetRow();
		csvRow* csv_root;
		csv_root = csvParseFile();
		json_object *json_root = json_object_new_array();
		csvToJson(csv_root, json_root);
		json_object_to_file(outputFile, json_root);
	}
	else if(operation == 6){ // JSON to CSV
		if(strcmp(getFileExtension(inputFile), "json") != 0){
			puts("Error: Input file is not a JSON file.");
			exit(0);
		}
		if(strcmp(getFileExtension(outputFile), "csv") != 0){
			puts("Error: Output file is not a CSV file.");
			exit(0);
		}
		json_object * jobj = json_tokener_parse(readJsonFile(inputFile));
		csvColumnNames = (char**)malloc(sizeof(char*) * 200);
		csvFile = (char*)malloc(sizeof(char) * 50000);
		FILE *fp = fopen(outputFile, "w");
		csvCreateColumnNamesByJson(jobj);
		int i = 0;
		while(i < csvColumnNumber){
			strcat(csvFile, csvColumnNames[i++]);
			if(i < csvColumnNumber)
				strcat(csvFile, ",");
			else
				strcat(csvFile, "\n");
		}
		jsonToCsv(jobj);
		csvFile[strlen(csvFile) - 1] = '\0';
		fprintf(fp,"%s", csvFile);
		fclose(fp);
	}
	else if(operation == 7){ // XML validate
		if(strcmp(getFileExtension(inputFile), "xml") != 0){
			puts("Error: Input file is not a XML file.");
			exit(0);
		}
		if(strcmp(getFileExtension(outputFile), "xsd") != 0){
			puts("Error: Validator file must be XSD.");
			exit(0);
		}
		xmlDocPtr doc;
		xmlSchemaPtr schema = NULL;
		xmlSchemaParserCtxtPtr ctxt;
		xmlLineNumbersDefault(1);
		ctxt = xmlSchemaNewParserCtxt(outputFile);
		schema = xmlSchemaParse(ctxt);
		xmlSchemaFreeParserCtxt(ctxt);
		doc = xmlReadFile(inputFile, NULL, 0);
		if(doc == NULL){
			printf("Error: %s is not found or it is not a valid xml file.\n", inputFile);
			exit(0);
		}
		xmlValidate(doc, schema);
		xmlSchemaCleanupTypes(); //cleanup the default xml schemas types library
    xmlCleanupParser(); //cleans memory allocated by the library itself
    xmlMemoryDump(); //memory dump
	}
	else{
		puts("Error: Wrong operation!");
		exit(0);
	}

	return 0;
}
