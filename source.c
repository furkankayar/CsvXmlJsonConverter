#include <stdio.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <string.h>
#include <json/json.h>




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

static void xmlToJson(xmlNode * xml_node, json_object * jsonObj){

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




int main()
{



 	xmlDocPtr doc = NULL;
	xmlNode *root_element = NULL;
  xmlNode *next_element = NULL;
	const char *Filename = "test.xml";
	doc = xmlParseFile(Filename);
	json_object *json_root = json_object_new_object();
	if (doc == NULL){
		printf("error: could not parse file %s\n", Filename);
	}
	else{

    root_element = xmlDocGetRootElement(doc);
		json_object *json_root = json_object_new_object();
		xmlToJson(root_element, json_root);
		printf("%s\n", json_object_to_json_string(json_root));
		json_object_to_file("test.json", json_root);

		//printf("%d\n", getXmlElementChildNumber(root_element));
  }
/*
	json_object *jobj = json_object_new_object();
	json_object *jstring = json_object_new_string("Test");
	json_object *jchild = json_object_new_object();
	json_object *str = json_object_new_string("Child baba");
	json_object_object_add(jobj, "Name", jstring);
	json_object_object_add(jchild, "-Element", str);
	json_object_object_add(jobj, "Child babcm", jchild);
	printf("%s\n", json_object_to_json_string(jobj));*/

	return (0);
}
