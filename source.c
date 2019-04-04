/* C program to convert file types each other.
   Operation 1 - CSV to XML
 	 Operation 2 - XML to CSV
	 Operation 3 - XML to JSON
	 Operation 4 - JSON to XML
	 Operation 5 - CSV to JSON
	 Operation 6 - JSON to CSV
	 Operation 7 - XML validates with XSD

	 Author: Furkan Kayar
*/

#include <stdio.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlschemastypes.h>
#include <string.h>
#include <json/json.h>

const char* jsonRootName; //Keeps root element of json file.
char** csvColumnNames; //It is a string array that keeps column names of csv file.
int csvColumnNumber = 0; //It keeps column number of a csv file.
char* csvFile;	//It keeps content of csv file.
int columnCount = 0; //It counts written value number when csv file being created.

typedef struct _csvRow{ //Keeps a row of csv file. And it keeps next row as linked list.

	char **columns;	//Values of columns. String array.
	struct _csvRow *next; //Pointer of next row.

}csvRow;


static char* getFileExtension(char *fileName){ //Function is used to learn type of given file. It is used to check if valid files are given.

	char *extension = (char*)malloc(sizeof(char) * 50);
	int i = strlen(fileName) - 1;
	while(fileName[i - 1] != '.') i--;
	int k = 0;
	while(i < strlen(fileName)){
		extension[k++] = fileName[i++];
	}

	return extension;
}

static int contains(char **strArr, char *str){ //Function that searches a string in a string array. If string is found, function returns 1.

	int i = 0;
	while(strArr[i] != NULL){
		if(strcmp(strArr[i], str) == 0)
			return 1;
		i++;
	}
	return 0;
}

static char* replaceAll(char* str, char oldCh, char newCh){ //Function that replaces all character with another character.
	char *copy = (char*)malloc(sizeof(char) * 500);
	strcpy(copy, str);
	int i = 0;
	for(i = 0 ; i < strlen(copy) ; i++) if(copy[i] == oldCh) copy[i] = newCh;
	return copy;
}

static char* trim(const char* str, int option){ //Function that cleans unwanted values from beginning and end of the string.

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

static char* readFile(char* fileName){ //Function that puts a file to a string.

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

static char* readJsonFile(char* fileName){ //Function that reads a json file. If json file has no root element, it adds a root element to json file.

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

static char** csvGetRow(){ //Function that splits a row of csv file and returns. It returns string array of row.

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

static csvRow* csvParseFile(){ //Function that creates a linked list from csv file by using struct _csvRow.

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

static char* jsonGetRootName(json_object *jsonObj){	//Function that gives the root name of given json tree.

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

static int getXmlElementChildNumber(xmlNode *node){ //Function that counts child number of xml node except "text" nodes. These "text" nodes are created by libxml library.
	int i = 0;
	node = node->children;
	while(node != NULL){
		if(strcmp(node->name, "text") != 0)
			i++;
		node = node->next;
	}
	return i;
}

static void csvCreateColumnNamesByJson(json_object *jsonObj){ //Function that decides csv column names by using a json file. If json object is not object and array, it can be a column in csv.

	int arrLen = 0;
	json_object *object;
	char * str;
	enum json_type type;
	json_object_object_foreach(jsonObj, key, val){

		type = json_object_get_type(val);
		switch(type){

			case json_type_object:
				csvCreateColumnNamesByJson(val); //If json object is object, it can not be in csv file. But the childs of it can. So, recursion starts.
			break;
			case json_type_array:

				arrLen = json_object_array_length(val);
				int i = 0;
				for(i = 0 ; i < arrLen ; i++){
					object = json_object_array_get_idx(val, i);
					csvCreateColumnNamesByJson(object); //If json object is array, it can not be in csv file. But its elements can. Its elements are checked recursively.
				}

			break;
			default:
				str = (char*)malloc(sizeof(char) * 200);
				strcpy(str, key);
				if(contains(csvColumnNames, str) != 1){
					csvColumnNames[csvColumnNumber] = (char*)malloc(sizeof(char) * 200);
					strcpy(csvColumnNames[csvColumnNumber], key); //Other all objects can be in csv file as new columns. So, key values is taken to csvColumnNames array.
					csvColumnNumber++; // Counting total column number.
				}
			break;
		}
	}
}

static void csvCreateColumnNamesByXml(xmlNode *xml_node){ //Function that decides csv column names by using a xml file. Only xml attributes can be a new column in csv file.

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
					strcpy(csvColumnNames[csvColumnNumber], str); // All attributes are new columns in csv.
					csvColumnNumber++;
				}
        attribute = attribute->next;
      }
    }
    csvCreateColumnNamesByXml(cur_node->children);
  }
}

static void xmlToJson(xmlNode * xml_node, json_object * jsonObj){ //Function that scans xml file recursively and creates a new json tree.

	xmlNode *cur_node = NULL;
	char lastName[250] = {'\0'};
	json_object *newObj = NULL;
	json_object *jsonArr = NULL;
  int isArrayCreated = 0;

	for (cur_node = xml_node; cur_node; cur_node = cur_node->next) {

		if(cur_node->type == XML_ELEMENT_NODE){
			if(getXmlElementChildNumber(cur_node) == 0){ // Means this is a text node.
				if(cur_node->children !=NULL){
					char *content = cur_node->children->content;
					int i = 0;
					for(i = 0 ; i < strlen(content); i++)
						if(content[i] == '\n') content[i] = ' ';

					if(cur_node->properties != NULL){ // Means that node has attributes.
						newObj = json_object_new_object();
						xmlAttr *attribute = cur_node->properties;
						if(attribute != NULL){
							do{
								json_object_object_add(newObj, attribute->name, json_object_new_string(attribute->children->content)); // All attributes are attached to json object.
								attribute = attribute->next;
							}while(attribute != NULL);
						}
						json_object_object_add(newObj, "#text", json_object_new_string(cur_node->children->content)); // Content is attached to json object.
				 	}
					else
						newObj = json_object_new_string(cur_node->children->content); // If xml node has a content but it does not have any attributes, only json string is created.
				}
				else{
					if(cur_node->properties != NULL){ // Attributes are checked if node is element node.
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

			xmlToJson(cur_node->children, newObj); // Recursion starts here to look all child nodes.
			xmlNode *nextNode = cur_node->next;
			while(nextNode && strcmp(nextNode->name, "text") == 0) nextNode = nextNode->next;
			if((nextNode && strcmp(cur_node->name, nextNode->name) == 0) || strcmp(cur_node->name, lastName) == 0){ // Next node is checked, if next node has same name with current node, new json array is created and nodes are inserted to these array.
    		if(isArrayCreated == 0){ // If this is first meeting with same name with next node, new array is created.
      		jsonArr = json_object_new_array();
        	json_object_object_add(jsonObj, cur_node->name, jsonArr);
        	json_object_array_add(jsonArr, newObj);
      		strcpy(lastName, cur_node->name);
        	isArrayCreated = 1;
      	}
      	else{ // If new array is already created, new object are inserted to array.
      		json_object_array_add(jsonArr, newObj);
      		strcpy(lastName, cur_node->name);
      	}
			}else{ // If next node does not have same name.
      	isArrayCreated = 0;
      	json_object_object_add(jsonObj, cur_node->name, newObj);
     	}
		}
	}
}

static void jsonToXml(json_object * jobj, xmlNodePtr xmlNode){ //Function that scans json file recursively and creates a new xml tree.

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
					xmlAddChildList(xmlNode, newNode); // New child is added.
					jsonToXml(object, newNode); // and content of new child will be added recursively.
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
					xmlAddChildList(xmlNode, newNode); // New child is added.
					jsonToXml(object, newNode); // and content of new child will be added recursively.
				}

			break;
			default: // If json type is not array, or object. Xml not can not have any child, it has only text content. So, there is not recursion here.

				xmlNewChild(xmlNode, NULL, BAD_CAST replaceAll(key, ' ', '_'), BAD_CAST trim(json_object_get_string(val),0));

			break;
		}
	}
}

static void csvToJson(csvRow* csvObj, json_object *jobj){ //Function that creates a json file iteratively by using my csv linked list.

	while(csvObj != NULL){
		json_object *row = json_object_new_object();
		int i = 0;
		while(csvObj->columns[i] != NULL){
			json_object * column = json_object_new_string(csvObj->columns[i]);
			json_object_object_add(row, csvColumnNames[i], column);
			i++;
		}
		json_object_array_add(jobj,row);
		csvObj = csvObj->next; // Next node of csv file.
	}
}

static void csvToXml(csvRow* csvObj, xmlNodePtr xmlNode){ //Function that creates a xml file iteratively by using my csv linked list.

	while(csvObj != NULL){

		xmlNodePtr newNode = xmlNewNode(NULL, BAD_CAST "row");
		int i = 0;
		while(csvObj->columns[i] != NULL){

			xmlNewProp(newNode, BAD_CAST replaceAll(csvColumnNames[i], ' ', '_'), BAD_CAST csvObj->columns[i]);
			//Uncomment next line if you want to make csv columns as new elements in xml file. Do not forget to delete previous line.
			//xmlNewChild(newNode, NULL, BAD_CAST replaceAll(csvColumnNames[i], ' ', '_'), BAD_CAST csvObj->columns[i]);

			i++;
		}
		xmlAddChildList(xmlNode, newNode);
		csvObj = csvObj->next;
	}
}

static void jsonToCsv(json_object *jsonObj){ //Function that creates a csv file by using json tree.

	int arrLen = 0;
	json_object *object;
	enum json_type type;
	json_object_object_foreach(jsonObj, key, val){

		type = json_object_get_type(val);
		switch(type){

			case json_type_object:

				jsonToCsv(val); // If json object is object, it can not be in csv file.

			break;
			case json_type_array:

				arrLen = json_object_array_length(val);
				int i = 0;
				for(i = 0 ; i < arrLen ; i++){ // Reads all objects in array.
					object = json_object_array_get_idx(val, i);
					jsonToCsv(object);
				}

			break;
			default:
				//All other object types can be in csv file.(Except array type and object type)
				while(strcmp(csvColumnNames[columnCount % csvColumnNumber], key) != 0){ // Aligns pointer and csv column to add value.
					strcat(csvFile, ",");
					columnCount++;
					if(columnCount % csvColumnNumber == 0){ // If line is end ,new line starts.
						csvFile[strlen(csvFile) - 1] = '\0';
						strcat(csvFile, "\n");
					}
				}
				strcat(csvFile, trim(json_object_to_json_string(val),1));
				strcat(csvFile, ",");
				columnCount++;
				if(columnCount % csvColumnNumber == 0){ // If line is end ,new line starts.
					csvFile[strlen(csvFile) - 1] = '\0';
					strcat(csvFile, "\n");
				}

			break;
		}
	}
}

static void xmlToCsv(xmlNode *xml_node){ //Function that creates a csv file by using xml tree. Only xml attributes can be new columns in csv file.

	xmlAttr *attribute = NULL;
  xmlNode *cur_node = NULL;
	char* str = NULL;

  for (cur_node = xml_node; cur_node; cur_node = cur_node->next) {
    if (cur_node->type == XML_ELEMENT_NODE) {
    	attribute = cur_node->properties;
    	while(attribute){ // Adds all attributes to csv file as columns.
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

static void xmlValidate(xmlDocPtr doc, xmlSchemaPtr schema){ // Function that validates xml file with xsd file.

	xmlSchemaValidCtxtPtr ctxt;
	int ret;

	ctxt = xmlSchemaNewValidCtxt(schema); //Create an xml schemas validation context.
  ret = xmlSchemaValidateDoc(ctxt, doc); //Validate a document tree in memory.
  if (ret == 0)
    printf("XML file validates.\n");
  else if (ret > 0) //Positive error code number.
	  printf("XML file fails to validate.\n");
  else
    printf("XML file validation generated an internal error.\n");

	xmlSchemaFreeValidCtxt(ctxt); //Free the resources associated to the schema validation context
  xmlFreeDoc(doc);

}


int main(int argc, char **argv){

	char *inputFile;	//Name of input file.
	char *outputFile; //Name of output/xsd file.
	int operation; // Number of operation.

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
	operation = argv[3][0] - 48;	// Numbers start from 48 in ascii table.


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
		while(i < csvColumnNumber){ // Column names are written to csv file.
			strcat(csvFile, csvColumnNames[i++]);
			if(i < csvColumnNumber)
				strcat(csvFile, ",");
			else
				strcat(csvFile, "\n");
		}
		xmlToCsv(root_element);
		csvFile[strlen(csvFile) - 1] = '\n';
		fprintf(fp,"%s", csvFile);
		fclose(fp);
		puts("Only xml attributes are written as new columns in csv.");
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
		json_object_to_file_ext(outputFile, json_root, 2);
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
		json_object_to_file_ext(outputFile, json_root,2);
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
		while(i < csvColumnNumber){ // Column names are written to csv file.
			strcat(csvFile, csvColumnNames[i++]);
			if(i < csvColumnNumber)
				strcat(csvFile, ",");
			else
				strcat(csvFile, "\n");
		}
		jsonToCsv(jobj);
		csvFile[strlen(csvFile) - 1] = '\n';
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
		xmlSchemaCleanupTypes(); //Cleanup the default xml schemas types library.
    xmlCleanupParser(); //Cleans memory allocated by the library itself.
    xmlMemoryDump(); //Memory dump.
	}
	else{
		puts("Error: Wrong operation!");
		exit(0);
	}

	return 0;
}
