#include <stdio.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <string.h>
#if defined(LIBXML_TREE_ENABLED) && defined(LIBXML_OUTPUT_ENABLED)

char *fgetstr(char *string, int n, FILE *stream)
{
    char *result;
    result = fgets(string, n, stream);
    if(!result)
	return(result);

    if(string[strlen(string) - 1] == '\n')
	string[strlen(string) - 1] = 0;

    return(string);
}

xmlNodePtr create_child(xmlNodePtr root_node, int attr_on) //if attr_on is 0 no attribute is added
{
    xmlNodePtr node = NULL;
    char cn[30],ccon[256];
    printf("Enter child name: ");
    scanf("%s",cn);
    getchar();//To handle newline
    printf("Enter child content: ");
    fgetstr(ccon,sizeof(ccon),stdin);
    if(strcmp(ccon , "\n") == 0)
	node = xmlNewChild(root_node, NULL, BAD_CAST cn,NULL);
    else
	node = xmlNewChild(root_node, NULL, BAD_CAST cn,BAD_CAST ccon);

    if(attr_on)
    {
	char attrname[30],attrval[256];
	printf("Enter child attribute name: ");
	scanf("%s",attrname);
	printf("Enter child attribute value: ");
	scanf("%s",attrval);
	xmlNewProp(node, BAD_CAST attrname, BAD_CAST attrval);
    }

    return node;
}


int main(int argc, char **argv)
{
    xmlDocPtr doc = NULL;
    xmlNodePtr root_node = NULL, node = NULL, node1 = NULL;
    xmlDtdPtr dtd = NULL;
    char buff[256];
    char rn[30];
    int i, j;

    LIBXML_TEST_VERSION;

    doc = xmlNewDoc(BAD_CAST "1.0");

    printf("Enter root name: ");
    scanf("%s",rn);

    root_node = xmlNewNode(NULL, BAD_CAST rn);
    xmlDocSetRootElement(doc, root_node);

    //Creates a DTD declaration.
    dtd = xmlCreateIntSubset(doc, BAD_CAST rn, NULL, BAD_CAST "tree2.dtd");

    //Creates new child nodes
    create_child(root_node , 0);
    node = create_child(root_node , 1);

    create_child(node , 0);

    //Dumping document to stdio or file
    xmlSaveFormatFileEnc(argc > 1 ? argv[1] : "-", doc, "UTF-8", 1);

    xmlFreeDoc(doc);
    xmlCleanupParser();
    xmlMemoryDump();
    return(0);
}
#else
int main(void)
{
    fprintf(stderr, "tree support not compiled in\n");
    exit(1);
}
#endif
