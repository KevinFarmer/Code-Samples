/*==================================================================
  queryProcess.c The query engine component of the tiny search engine

  Project name: Tiny Search Engine
  Component name: Query
	
  Primary Author: Kevin Farmer
  Date: 5/18/15

  Contains necessary functions for the Query Engine

  Special considerations:  
  Compile with given Makefile
	
======================================================================*/
#define _GNU_SOURCE

// ---------------- Open Issues 

// ---------------- System includes e.g., <stdio.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// ---------------- Local includes  e.g., "file.h"
#include "query.h"
#include "../../util/tseutil.h"

// ---------------- Constant definitions 

// ---------------- Macro definitions
#define BUF_SIZE 100
#define MEM_ERR "Out of memory! Exiting...\n"
//#define DEBUG

// ---------------- Structures/Types 

// ---------------- Private variables 

// ---------------- Private prototypes 

/*====================================================================*/
// Checks the line for invalid input
int checkLine(const char *line)
{
	char *checkLine = malloc(strlen(line)+1);
	char *token;
	char *curWord;
	int argCount = 1;
	
	strcpy(checkLine, line);

	token = strtok(checkLine, " ");
	while (token != NULL) {
		if (argCount == 1) { //if it starts w/ AND or OR
			if (strcmp(token, "AND") == 0) {
				free(checkLine);
				return 1;
			}
			if (strcmp(token, "OR") == 0){
				free(checkLine);
				return 1;
			}
		} else if (strcmp(curWord, "AND") == 0 || strcmp(curWord, "OR") == 0){
			if (strcmp(token, "AND") == 0) {
				free(checkLine);
				return 1;
			}
			if (strcmp(token, "OR") == 0){
				free(checkLine);
				return 1;
			}
		}

		argCount++;
		curWord = token;
		token = strtok(NULL, " ");
	}

	//if it ends in AND or OR
	if (strcmp(curWord, "AND") == 0 || strcmp(curWord, "OR") == 0){	
		free(checkLine);
		return 1;
	}

	free(checkLine);
	return 0;
}


//Process a given query input line
int processQuery(HashTable *index, char *line, char *dir)
{
	char *token;
	int inputArgs = 0;
	List *inputList = malloc(sizeof(List));
	List *orList;
	List *finalDocList;

	if (inputList == NULL)
		return 1;
	
	initList(inputList);

	// Separates the query, and removes any AND 's
	token = strtok(line, " ");
	while (token != NULL) {
		if (strcmp(token, "AND") != 0) {
			if (strcmp(token, "OR") != 0)
				NormalizeWord(token);
			appendToList(inputList, (void *) token);
			inputArgs++;
		}
		token = strtok(NULL, " ");
	}

	//If nothing added to the list
	if(!listHasNext(inputList)) {
		fprintf(stderr, "\nNo found matches.\n\n");
		return 1;
	}

	//If only one thing added to the list
	if (inputArgs == 1) {
		WordNode *word = findWordNode(index, popFromList(inputList));
		if (word == NULL) {
			freeList(inputList);
			free(inputList);
			printf("\nNo found matches.\n\n");
			return 1;
		} else {
			word = copyWordNode(word);
			finalDocList = word->docs;
			free(word);
		}
	} else {
		orList = createOrList(index, inputList);
		if(!listHasNext(orList)) {
			freeList(orList);
			free(orList);
			printf("\nNo found matches.\n\n");
			return 1;
		}

		finalDocList = processOrList(orList);
	}

	freeList(inputList);
	free(inputList);

	sortDocs(finalDocList);


	// Print the results
	DocumentNode *docNode;
	char *url;
	int printed = 0;
	printf("\n");
	while(listHasNext(finalDocList)) {
		docNode = popFromList(finalDocList);
		url = getFileUrl(docNode->doc_id, dir);
		if (url) {
			printed = 1;
			printf("Document ID:%i URL:%s", docNode->doc_id, url);
#ifdef DEBUG
			printf("freq: %i\n", docNode->freq);
#endif
			free(url);
		}
		free(docNode);
	}
	if (printed == 0)
		printf("No found matches.\n");
	printf("\n");
	
	if (inputArgs != 1) {
		freeList(orList);
		free(orList);
	}
		

	freeList(finalDocList);
	free(finalDocList);


	return 0;
}


//Creates an orList from the input list by processing all of the and operators
List * createOrList(HashTable *index, List *inputList)
{
	char *word;
	WordNode *curNode = NULL;
	WordNode *wordNode = NULL;
	List *orList = malloc(sizeof(List));
	HashTable *duplicates = malloc(sizeof(HashTable));
	char *hashWord;
	initList(orList);
	initHash(duplicates);

	//goes through the input, and creates a single list of WordNodes
	//to do the OR operator on
	while(listHasNext(inputList)) {
		word = popFromList(inputList);
		if (checkHash(duplicates, (void *) word) != 0 ) {
			hashWord = malloc(strlen(word)+1);
			strcpy(hashWord, word);
			addToHash(duplicates, (void *) hashWord);

			if (strcmp(word, "OR") == 0) {
				if (curNode != NULL) {
					if (curNode->word != NULL)
						curNode = copyWordNode(curNode);
					appendToList(orList, curNode);
					freeHash(duplicates);
					initHash(duplicates);
					curNode = NULL;
				}
			} else if (curNode == NULL) {
				curNode = findWordNode(index, word);
				if (curNode == NULL) {
					while (listHasNext(inputList) && 
					       strcmp(word, "OR")  != 0) {
						word = popFromList(inputList);
					}
					curNode = NULL;
				}
			
			} else {
				wordNode = findWordNode(index, word);
				if (wordNode != NULL) {
					curNode = processAND(curNode, 
							     wordNode);
				} else {
					while (listHasNext(inputList) && 
					       strcmp(word, "OR")  != 0) {
						word = popFromList(inputList);
					}
					curNode = NULL;
				}
			}
		}
	}
	if (curNode != NULL) {
		if (curNode->word != NULL)
			curNode = copyWordNode(curNode);
		appendToList(orList, curNode);
	}
	freeHash(duplicates);
	free(duplicates);


	return orList;
}


//Processes the orList, creating a single combined list
List * processOrList(List *orList)
{
	List *finalList;
	WordNode *orNode = NULL;
	WordNode *tempNode;
	List *tempDocs;

	//process the OR operator on the WordNodes in this list
	while(listHasNext(orList)) {
		if (orNode == NULL) {
			orNode = popFromList(orList);
			finalList = orNode->docs;
			free(orNode);
		} else {
			tempNode = popFromList(orList);
			tempDocs = tempNode->docs;
			finalList = processOR(finalList, tempDocs);
			free(tempNode);
		}
	}

	return finalList;
}


// Returns the given WordNode for a word, or NULL if non-existant
WordNode * findWordNode(HashTable *index, char *word)
{
	int hash = JenkinsHash(word, MAX_HASH_SLOT);
	HashTableNode *hashNode = index->table[hash];

	if (hashNode->data == NULL) { // No words in hash slot
		return NULL;
	} else {
		WordNode *wordNode = (WordNode *) hashNode->data;
		WordNode *nextNode;

		// checks the first WordNode
		if (strcmp(wordNode->word, word) == 0) {
			return wordNode;
		} else {
			nextNode = wordNode->next;
		}

		// Loops through and checks the rest
		while (nextNode != NULL) {
			if (strcmp(nextNode->word, word) == 0) {
				return nextNode;
			}
			wordNode = nextNode;
			nextNode = wordNode->next;
		}
	}

	return NULL;
}



// Performs the AND operator on the two nodes, and returns a single WordNode
WordNode * processAND(WordNode *word1, WordNode *word2)
{
#ifdef DEBUG
	printf("Combining %s %s\n", word1->word, word2->word);
#endif

	WordNode *newNode = malloc(sizeof(WordNode));
	List *docList = malloc(sizeof(List));

	if (newNode == NULL || docList == NULL)
		return NULL;
	initList(docList);
	newNode->next = NULL;
	newNode->word = NULL;
	newNode->numDocs = 0;
	newNode->docs = docList;

	List *list1 = word1->docs;
	List *list2 = word2->docs;
	ListNode *node1 = list1->head->next;
	ListNode *node2 = list2->head->next;
	DocumentNode *doc1;
	DocumentNode *doc2;

	DocumentNode *newDoc;
	while (node1 != list1->tail && node2 != list2->tail) {
	
		doc1 = (DocumentNode *) node1->data;
		doc2 = (DocumentNode *) node2->data;

#ifdef DEBUG
		printf("1: id-%i freq-%i\n", doc1->doc_id, doc1->freq);
		printf("2: id-%i freq-%i\n\n", doc2->doc_id, doc2->freq);
#endif

		if (doc1->doc_id == doc2->doc_id) {
			newDoc = malloc(sizeof(DocumentNode));
			newDoc->doc_id = doc1->doc_id;
			newDoc->freq = doc1->freq + doc2->freq;
			newNode->numDocs++;
			appendToList(docList, newDoc);

			node1 = node1->next;
			node2 = node2->next;
		} else if (doc1->doc_id < doc2->doc_id) {
			node1 = node1->next;
		} else {
			node2 = node2->next;
		}
	}

	return newNode;
}


//Combines the two lists into a single list based on the OR operator
List * processOR(List *list1, List *list2)
{
	List *docList = malloc(sizeof(List));
	initList(docList);

	ListNode *node1 = list1->head->next;
	ListNode *node2 = list2->head->next;
	DocumentNode *doc1;
	DocumentNode *doc2;

#ifdef DEBUG
	printf("Processing OR list...\n\n");
#endif

	DocumentNode *newDoc;
	while (node1 != list1->tail || node2 != list2->tail) {
	
		doc1 = (DocumentNode *) node1->data;
		doc2 = (DocumentNode *) node2->data;

#ifdef DEBUG
		if (doc1 != NULL)
			printf("1: id-%i freq-%i\n", doc1->doc_id, doc1->freq);
		if (doc2 != NULL)
			printf("2: id-%i freq-%i\n\n", doc2->doc_id, doc2->freq);
#endif
		newDoc = calloc(1, sizeof(DocumentNode));
		
		if (doc2 == NULL) {
			newDoc->doc_id = doc1->doc_id;
			newDoc->freq = doc1->freq;
			appendToList(docList, newDoc);
			node1 = node1->next;
		} else if (doc1 == NULL) {
			newDoc->doc_id = doc2->doc_id;
			newDoc->freq = doc2->freq;
			appendToList(docList, newDoc);
			node2 = node2->next;
		} else if (doc1->doc_id < doc2->doc_id){
			newDoc->doc_id = doc1->doc_id;
			newDoc->freq = doc1->freq;
			appendToList(docList, newDoc);
			node1 = node1->next;
		} else if (doc1->doc_id > doc2->doc_id){
			newDoc->doc_id = doc2->doc_id;
			newDoc->freq = doc2->freq;
			appendToList(docList, newDoc);
			node2 = node2->next;
		} else {
			newDoc->doc_id = doc1->doc_id;
			newDoc->freq = doc1->freq + doc2->freq;
			appendToList(docList, newDoc);

			node1 = node1->next;
			node2 = node2->next;
		}
	}

	freeList(list1);
	free(list1);
	freeList(list2);
	free(list2);

	return docList;
}

// Makes a copy of the given WordNode and returns it
WordNode * copyWordNode(WordNode *curWord)
{
	WordNode *newNode = malloc(sizeof(WordNode));
	List *docList = malloc(sizeof(List));

	if (newNode == NULL || docList == NULL)
		return NULL;

	initList(docList);
	newNode->next = NULL;
	newNode->word = NULL;
	newNode->numDocs = curWord->numDocs;
	newNode->docs = docList;

	List *curList = curWord->docs;
	ListNode *curNode = curList->head->next;
	DocumentNode *curDoc;
	DocumentNode *newDoc;

	while (curNode != curList->tail) {
		curDoc = (DocumentNode *) curNode->data;

		newDoc = malloc(sizeof(DocumentNode));
		newDoc->doc_id = curDoc->doc_id;
		newDoc->freq = curDoc->freq;
		appendToList(docList, newDoc);

		curNode = curNode->next;
	}

	return newNode;
}


//Sorts the doc array, and displays the results
void sortDocs(List *docList)
{
	ListNode *curNode = docList->head;
	int count = 0;
	while (curNode->next != docList->tail) {
		count++;
		curNode = curNode->next;
	}

#ifdef DEBUG
	printf("Count: %i\n", count);
#endif

	//Copy the list into an array for sorting
	DocumentNode *list[count];
	DocumentNode *temp = malloc(sizeof(DocumentNode));
	int i = 0;
	while (listHasNext(docList)) {
		list[i] = (DocumentNode *) popFromList(docList);
		i++;
	}

#ifdef DEBUG
	printf("Before sorting:\n");
	for (int i = 0; i < count; i++)
		printf("%i %i\n", list[i]->doc_id, list[i]->freq);
#endif

	for (int i = 1; i < count; i++) {
		int j = i;
		while (j > 0 && list[j-1]->freq > list[j]->freq) {
			//swap the nodes
			temp->doc_id = list[j]->doc_id;
			temp->freq = list[j]->freq;
			list[j]->doc_id = list[j-1]->doc_id;
			list[j]->freq = list[j-1]->freq;
			list[j-1]->doc_id = temp->doc_id;
			list[j-1]->freq = temp->freq;

			j--;
		}
	}
	free(temp);

#ifdef DEBUG
	printf("After sorting:\n");
	for (int i = 0; i < count; i++)
		printf("%i %i\n", list[i]->doc_id, list[i]->freq);
#endif

	//Adds them to the list with highest freq first
	for (int i = count-1; i >= 0; i--) {
		appendToList(docList, list[i]);
	}



}


//Retrieves the url from the given file
char * getFileUrl(int docId, char *dir)
{
	char *path;
	char *url;
	size_t nbytes = BUF_SIZE;
	FILE *fp;

	path = malloc(strlen(dir)+sizeof(docId)+1);

	sprintf(path, "%s%i", dir, docId);

	if ((fp = fopen(path, "r")) == NULL)
		return NULL;

	url = malloc(sizeof(char) * (BUF_SIZE+1));
	if (url == NULL) {
		fprintf(stderr, MEM_ERR);
		return NULL;
	}
	url[0] = '\0';

	getline(&url, &nbytes, fp);

	free(path);
	fclose(fp);

	return url;
}
