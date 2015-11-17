/* 	query.h Header file for the query engine

	Project name: Tiny Search Engine
	Component name: Query

	This file contains functin declarations for query.c
	
	Primary Author:	Kevin Farmer
	Date Created: 5/18/15
	
======================================================================*/

#ifndef QUERY_H
#define QUERY_H

// ---------------- Prerequisites e.g., Requires "math.h"
#include "../../util/tseutil.h"

// ---------------- Constants

// ---------------- Structures/Types

// ---------------- Public Variables

// ---------------- Prototypes/Macros


/*
 *@index: a valid index, from indexFromFile()
 *@line: a search line
 *@dir: the directory where corresponding crawler files are located
 *
 *Takes in a query and displays links by relevance
 *
 */
int processQuery(HashTable *index, char *line, char *dir);


/*
 *@line: the input to be checked
 *
 * Checks the line for invalid input
 *
 * Returns 1 if line is invalid, 0 otherwise
 */
int checkLine(const char *line);


/*
 *@index: a valid index
 *@word: the word to find in the index
 *
 * Returns the given WordNode for a word, or NULL if non-existant
 */
WordNode * findWordNode(HashTable *index, char *word);

/*
 *@index: creates an orList of WordNodes
 *@inputList: a list of words and OR operators
 *
 *Creates an orList from the input list by processing all of the and operators
 *
 *Returns a list of WordNodes that the OR operator must be performed on
 */
List * createOrList(HashTable *index, List *inputList);


/*
 *@orList: a list of WordNodes that the OR operator must be performed on
 *
 * Processes the orList, creating a single combined list
 *
 * Returns a list of DocumentNodes for the given query
 */
List * processOrList(List *orList);


/*
 *@node1: an existing WordNode
 *@node2: an existing WordNode
 *
 * Performs the AND operator on the two nodes
 *
 * Returns a single WordNode with a new DocumentNode List
 */
WordNode * processAND(WordNode *node1, WordNode *node2);


/*
 *@list1: a List of DocumentNodes
 *@list2: a List of DocumentNodes
 *
 * Combines the two lists into a single list based on the OR operator
 *
 * Returns the combined list of DocumentNodes
 */
List * processOR(List *list1, List *list2);


/*
 *@curWord: the WordNode to be copied
 *
 * Makes a copy of the given WordNode
 *
 * Returns a pointer to the newly made copy
 */
WordNode * copyWordNode(WordNode *curWord);


/*
 *@docList: a List of DocumentNodes to be sorted
 *
 * Sorts the docList from low to high frequency
 */
void sortDocs(List *docList);


/*
 *@docId: the id of the desired file
 *@dir: the directory the file is located in
 *
 * Retrieves the url from the given file
 *
 * Returns the found url, or NULL if an error occured
 */
char * getFileUrl(int docId, char *dir);


#endif // QUERY_H

