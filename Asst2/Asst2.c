#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<pthread.h>
#include<dirent.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<errno.h>
#include<unistd.h>
#include<fcntl.h>
#include<ctype.h>
#include<math.h>

//struct representation of a token.
typedef struct tokenNode
{
  char * token;
  double freq;
  struct tokenNode * nextToken;
} tokenNode;

//struct representation of a file.
typedef struct fileNode
{
  char * filePath;
  int numTokens;
  struct fileNode * nextFile;
  struct tokenNode * nextToken;
} fileNode;
//struct representation of pthread argument list.
typedef struct argList
{
  char * currPath;
  fileNode * shared;
  pthread_mutex_t * mutex;
} argList;
//struct representation of final Jenson-Shannon Distance output.
typedef struct jsdNode
{
  char * file1;
  char * file2;
  double jsd;
  int numTokens;
  struct jsdNode * nextJSD;
} jsdNode;
//Given a char * and double, mallocs a new tokenNode and returns a pointer to it.
tokenNode * createTokenNode(char * t, double f)
{
  tokenNode * n = (tokenNode*)malloc(sizeof(tokenNode));
  n->token = t;
  n->freq = f;
  n->nextToken = NULL;
  return n;
}
//Given a char * and int, mallocs a new fileNode and returns a pointer to it.
fileNode * createFileNode(char * f, int num)
{
  fileNode * n = (fileNode*)malloc(sizeof(fileNode));
  n->filePath = f;
  n->numTokens = num;
  n->nextFile = NULL;
  n->nextToken = NULL;
  return n;
}
//Given a char * and pointers to the shared resources (data struct and mutex), mallocs a new argList and returns a pointer to it.
argList * createArgList(char * path, fileNode * shared, pthread_mutex_t * m)
{
  argList * a = (argList*)malloc(sizeof(argList));
  a->currPath = path;
  a->shared = shared;
  a->mutex = m;
  return a;
}
//Given two char * pointers corresponding to the paths of two different files, the Jenson-Shannon Distance of the two files and the combined number of tokens of both files, mallocs a jsdNode and returns a pointer to it.
jsdNode * createJSDNode(char * file1, char * file2, double jsd, int numTokens)
{
  jsdNode * j = (jsdNode*)malloc(sizeof(jsdNode));
  j->file1 = file1;
  j->file2 = file2;
  j->jsd = jsd;
  j->numTokens = numTokens;
  j->nextJSD = NULL;
  return j;
}
//Prints a linked list of tokenNodes. 
void printTokens(tokenNode * start)
{
  tokenNode * ptr = start;
  while (ptr != NULL)
    {
      printf("%s: %f\n", ptr->token, ptr->freq);
      ptr = ptr->nextToken;
    }
}
//Prints a linked list of fileNodes.
void printFiles(fileNode * start)
{
  fileNode * ptr = start;
  while (ptr != NULL)
    {
      printf("%s: %d\n", ptr->filePath, ptr->numTokens);
      printTokens(ptr->nextToken); 
      ptr = ptr->nextFile;
    }
}
//Prints a linked list of jsdNodes.
void printJSDList(jsdNode * start)
{
  jsdNode * ptr = start;
  while (ptr != NULL)
    {
      //White for (0.3, infinity).
      if (ptr->jsd > 0.3) printf("\033[0;37m");
      //Blue for (0.25, 0.3].
      else if (ptr->jsd > 0.25) printf("\033[0;34m");
      //Cyan for (0.2, 0.25].
      else if (ptr->jsd > 0.2) printf("\033[0;36m");
      //Green for (0.15, 0.2].
      else if (ptr->jsd > 0.15) printf("\033[0;32m");
      //Yellow for (0.1, 0.15].
      else if (ptr->jsd > 0.1) printf("\033[0;33m");
      //Red for [0, 0.1].
      else printf("\033[0;31m");

      printf("%f ", ptr->jsd);
      printf("\033[m");
      printf("\"%s\" and \"%s\"\n", ptr->file1, ptr->file2);      
      ptr = ptr->nextJSD;
    }
}
//Frees a linked list of jsdNodes.
void freeJSD(jsdNode * start)
{
  jsdNode * ptr = start;
  jsdNode * prev = NULL;
  while (ptr != NULL)
    {
      prev = ptr;
      ptr = ptr->nextJSD;
      free(prev);
    }
}
//Frees a linked list of tokenNodes and the token corresponding to each tokenNode.
void freeTokens(tokenNode * start)
{
  tokenNode * ptr = start;
  tokenNode * prev = NULL;
  while (ptr != NULL)
    {
      prev = ptr;
      ptr = ptr->nextToken;
      free(prev->token);
      free(prev);
    }
}
//Frees a linked list of fileNodes, the linked list of tokenNodes corresponding to each fileNode, and the filePath corresponding to each fileNode.
void freeFiles(fileNode * start)
{
  fileNode * ptr = start;
  fileNode * prev = NULL;
  while (ptr != NULL)
    {
      prev = ptr;
      ptr = ptr->nextFile;
      freeTokens(prev->nextToken);
      free(prev->filePath);
      free(prev);
    }
}
//Frees a mean list, which is a linked list of tokenNodes. Frees each tokenNode struct, but not the corresponding token.
void freeMeanList(tokenNode * start)
{
  tokenNode * ptr = start;
  tokenNode * prev = NULL;
  while (ptr != NULL)
    {
      prev = ptr;
      ptr = ptr->nextToken;
      free(prev);
    }
}
//Frees and array of pointers to argList structs and each struct's corresponding currPath. (Used to free the structs created for each directory pthread.)
void freeDirStructs(argList ** start, int NUMBER_OF_DIRECTORIES)
{
  int i;
  for (i = 0; i < NUMBER_OF_DIRECTORIES; i++)
    {
      free(start[i]->currPath);
      free(start[i]);
    }
}
//Frees and array of pointers to argList structs only. (Used to free the structs created for each file pthread.)
void freeFileStructs(argList ** start, int NUMBER_OF_FILES)
{
  int i;
  for (i = 0; i < NUMBER_OF_FILES; i++)
    {
      free(start[i]);
    }
}
//Given three char * pointers, concatenates their associated strings and returns a pointer to the new string.
char * concatenate(char * s1, char * s2, char * s3)
{
  size_t size1 = strlen(s1);
  size_t size2 = strlen(s2);
  size_t size3 = strlen(s3);
  char * newString = malloc(size1 + size2 + size3 + 1);
  memcpy(newString, s1, size1);
  memcpy(newString + size1, s2, size2);
  memcpy(newString + size1 + size2, s3, size3 + 1);
  return newString;
}
//Given a char * that corresponds to a valid file path, opens the file and tokenizes it.
tokenNode * tokenize(FILE * fp, fileNode * file)
{
  tokenNode * tokenList = NULL;
  char tokenStart = fgetc(fp);
  int tokenCount = 0;
  while(tokenStart != EOF)
    {
      if (isalpha(tokenStart) || tokenStart == '-')
	{
	  tokenCount++;
	  char tokenEnd = fgetc(fp);
	  size_t tokenSize = 1;
	  size_t tokenSizeWithNoise = 1;
	  while (tokenEnd != EOF && !isspace(tokenEnd))
	    {
	      tokenSizeWithNoise++;
	      if (isalpha(tokenEnd) || tokenEnd == '-')
		{
		  tokenSize++;
		}
	      tokenEnd = fgetc(fp);
	    }
	  //Return to the beginning of the token.
	  fseek(fp, -tokenSizeWithNoise-1, SEEK_CUR);

	  //malloc the token.
	  char * word = (char*)malloc(sizeof(char)*(tokenSize + 1));

	  //Copies characters into word array.
	  int i;
	  int wordIndex = 0;
	  tokenEnd = fgetc(fp);
	  while (tokenEnd != EOF && !isspace(tokenEnd))
	    {
	      if (isalpha(tokenEnd) || tokenEnd == '-')
		{
		  word[wordIndex] = tolower(tokenEnd);
		  wordIndex++;
		}
	      tokenEnd = fgetc(fp);
	    }
	  word[wordIndex] = '\0';

	  //Updates a running tokenList in alphabetical order.
	  if(tokenList == NULL || strcmp(word, tokenList->token) < 0)
	    {
	      tokenNode * newNode = createTokenNode(word, 1);
	      newNode->nextToken = tokenList;
	      tokenList = newNode;
	    }
	  else
	    {
	      tokenNode * prev = NULL;
	      tokenNode * ptr = tokenList;
	      while (ptr != NULL)
		{
		  if (strcmp(word, ptr->token) > 0)
		    {
		      prev = ptr;
		      ptr = ptr->nextToken;
		    }
		  else if (strcmp(word, ptr->token) < 0)
		    {
		      tokenNode * newNode = createTokenNode(word, 1);
		      newNode->nextToken = ptr;
		      prev->nextToken = newNode;
		      break;
		    }
		  else
		    {
		      ptr->freq = ptr->freq + 1;
		      free(word);
		      break;
		    }
		}
	      if (ptr == NULL)
		{
		  prev->nextToken = createTokenNode(word, 1);
		}
	    }
	}
      tokenStart = fgetc(fp);
    }

  //Calculates the discrete probability of each token.
  tokenNode * curr = tokenList;
  while (curr != NULL)
  {
    curr->freq = curr->freq/tokenCount;
    curr = curr->nextToken;
  }
  //Stores the total token count in the fileNode.
  file->numTokens = tokenCount;
  
  return tokenList;
}
//File handler. Casts the void * paramter to an argList * pointer and updates the shared data structure. Then, makes a call to tokenize().
void * fileHandler(void * v)
{
  argList * arguments = (argList*)v;
  FILE * fp = fopen(arguments->currPath, "r");
  if (fp != NULL)
    {
      //Success: the file can be opened.

      //Locks the mutex.
      pthread_mutex_lock(arguments->mutex);

      //Updates the shared data struct.
      fileNode * ptr = arguments->shared;
      while (ptr->nextFile != NULL)
	{
	  ptr = ptr->nextFile;
	}
      ptr->nextFile = createFileNode(arguments->currPath, 0);

      //Unlocks the mutex.
      pthread_mutex_unlock(arguments->mutex);

      //Tokenizes the file.
      ptr->nextFile->nextToken = tokenize(fp, ptr->nextFile);

      fclose(fp);
    }
  else
    {
      //Failure: the file cannot be opened.
      printf("WARNING: %s could not be opened.\n", arguments->currPath);
    }
  
  return NULL;
}
//Directory handler. Spawns pthreads for each subdirectory and file present.
void * directoryHandler(void * v)
{
  argList * arguments = (argList*)v;
  DIR * currentDir = opendir(arguments->currPath);

  //  printf("We have arrived at %s\n", arguments->currPath);
  if (currentDir)
    {
      //Success: the directory can be opened.

      struct dirent * curr = readdir(currentDir);
      int NUMBER_OF_DIRECTORIES = 0;
      int NUMBER_OF_FILES = 0;

      //Counts the number of threads we'll have to spawn for all files and directories.
      while (curr != NULL)
	{
	  if (curr->d_type == DT_DIR &&
	      strcmp(".", curr->d_name) != 0 &&
	      strcmp("..", curr->d_name) != 0) NUMBER_OF_DIRECTORIES++;

	  else if (curr->d_type == DT_REG) NUMBER_OF_FILES++;

	  curr = readdir(currentDir);
	}

      //mallocs an array to store the pthreads.
      pthread_t * threadArr = (pthread_t *)malloc(sizeof(pthread_t) * (NUMBER_OF_DIRECTORIES + NUMBER_OF_FILES));
      int threadIndex = 0;

      //mallocs arrays to store the directory struct and the file struct pointers.
      argList ** dirStructs = (argList **)malloc(sizeof(argList *) * NUMBER_OF_DIRECTORIES);
      argList ** fileStructs = (argList **)malloc(sizeof(argList *) * NUMBER_OF_FILES);
      int dirIndex = 0;
      int fileIndex = 0;

      //Moves back to the first entry of the directory.
      rewinddir(currentDir);

      //Begins reading the directory once again.
      curr = readdir(currentDir);
      
      while (curr != NULL)
	{
	  //Checks whether curr is a directory and not called "." or "..".
	  if (curr->d_type == DT_DIR &&
	      strcmp(".", curr->d_name) != 0 &&
	      strcmp("..", curr->d_name) != 0)
	    {
	      //Concatenates the directory name to the current path.
	      char * newPath = concatenate(arguments->currPath, curr->d_name, "/");
	      
	      //Creates a struct to hold the directory path, shared data struct pointer and mutex pointer.
	      argList * dirArgs = createArgList(newPath, arguments->shared, arguments->mutex);

	      //Stores struct pointer to free when done.
	      dirStructs[dirIndex] = dirArgs;
	      dirIndex++;
	      
	      //Creates pthread to run the directory handling procedure.
	      pthread_create(&threadArr[threadIndex], NULL, directoryHandler, (void*)dirArgs);
	      threadIndex++;
	    }
	  else if (curr->d_type == DT_REG)
	    {
	      //Concatenates the file name to the current path.
	      char * newPath = concatenate(arguments->currPath, curr->d_name, "");

	      //Creates a struct to hold the file path, shared data struct pointer and mutex pointer.
	      argList * fileArgs = createArgList(newPath, arguments->shared, arguments->mutex);

	      //Stores struct pointer to free when done.
	      fileStructs[fileIndex] = fileArgs;
	      fileIndex++;

	      //Creates pthread to run the file handling procedure.
	      pthread_create(&threadArr[threadIndex], NULL, fileHandler, (void*)fileArgs);
	      threadIndex++;
	    }
	  curr = readdir(currentDir);
	}
      //Joins all pthreads.
      int x;
      for (x = 0; x < NUMBER_OF_DIRECTORIES + NUMBER_OF_FILES; x++)
	{
	  pthread_join(threadArr[x], NULL);
	}

      //Frees all pthread structs, as they are no longer needed.
      free(threadArr);
      freeDirStructs(dirStructs, NUMBER_OF_DIRECTORIES);
      freeFileStructs(fileStructs, NUMBER_OF_FILES);
      free(dirStructs);
      free(fileStructs);
      
      closedir(currentDir);
    }
  else
    {
      //Failure: the directory cannot be opened.
      printf("WARNING: %s could not be opened.\n", arguments->currPath);
    }
  return NULL;
}
//Computes the mean distribution of two different token lists. Returns a pointer to the head of the mean distribution.
tokenNode * meanConstruction(tokenNode * list1, tokenNode * list2)
{
  //Creates a dummy header node.
  tokenNode * result = createTokenNode("dummy", -1);
  tokenNode * ptr = result;

  //Iterates as long as there are non-null values in both lists.
  while (list1 != NULL && list2 != NULL)
    {
      if (strcmp(list1->token, list2->token) < 0)
	{
	  ptr->nextToken = createTokenNode(list1->token, list1->freq/2);
	  list1 = list1->nextToken;
	}
      else if (strcmp(list1->token, list2->token) > 0)
	{
	  ptr->nextToken = createTokenNode(list2->token, list2->freq/2);
	  list2 = list2->nextToken;
	}
      else
	{
	  ptr->nextToken = createTokenNode(list1->token, (list1->freq + list2->freq)/2);
	  list1 = list1->nextToken;
	  list2 = list2->nextToken;
	}
      ptr = ptr->nextToken;
    }
  //Finishes computing the mean list on the non-null list.
  if (list1 == NULL)
    {
      while (list2 != NULL)
	{
	  ptr->nextToken = createTokenNode(list2->token, list2->freq/2);
	  list2 = list2->nextToken;
	  ptr = ptr->nextToken;
	}
    }
  else if (list2 == NULL)
    {
      while (list1 != NULL)
	{
	  ptr->nextToken = createTokenNode(list1->token, list1->freq/2);
	  list1 = list1->nextToken;
	  ptr = ptr->nextToken;
	}
    }
  tokenNode * newHead = result->nextToken;
  free(result);
  return newHead;
}
//Given a tokenNode list and its corresponding mean list, computes the Kullback-Leibler Divergence.
double kld(tokenNode * list, tokenNode * mean)
{
  double kld = 0;
  
  while(list != NULL && mean != NULL)
    {
      if (strcmp(list->token, mean->token) < 0)
	{
	  list = list->nextToken;
	}
      else if (strcmp(list->token, mean->token) > 0)
	{
	  mean = mean->nextToken;
	}
      else
	{
	  kld += list->freq * log10(list->freq / mean->freq);
	  list = list->nextToken;
	  mean = mean->nextToken;
	}
    }
  return kld;
}
//Given a pointer to the shared struct, performs the analysis on the list.
void analysis(fileNode * sharedStruct)
{
  //Returns if the shared struct contains no data.
  if (sharedStruct->nextFile == NULL)
    {
      printf("ERROR: No files were found.\n");
      return;
    }
  //Returns if the shared struct only contains a single file.
  if (sharedStruct->nextFile->nextFile == NULL)
    {
      printf("ERROR: Cannot perform analysis on a single file.\n");
      return;
    }
  
  jsdNode * finalResults = NULL;
  fileNode * i;

  //Iterates through all files in the shared struct pairwise (without reflexive comparisons).
  for (i = sharedStruct->nextFile; i != NULL; i = i->nextFile)
    {
      fileNode * j;
      for (j = i->nextFile; j != NULL; j = j->nextFile)
	{

	  //Creates the mean list.
	  tokenNode * meanList = meanConstruction(i->nextToken, j->nextToken);

	  //Computes the Kullback-Leibler Divergences of both files. 
	  double kld1 = kld(i->nextToken, meanList);
	  double kld2 = kld(j->nextToken, meanList);

	  //Computes the Jenson-Shannon Distance.
	  double jsd = (kld1 + kld2)/2;

	  //Computes the numbers of tokens in file1 + the number of tokens in file 2.
	  int totalTokens = i->numTokens + j->numTokens;

	  //Creates a new JSD Node.
	  jsdNode * newNode;
	  if (i->numTokens > j->numTokens) newNode = createJSDNode(j->filePath, i->filePath, jsd, totalTokens);
	  else newNode = createJSDNode(i->filePath, j->filePath, jsd, totalTokens);

	  //Inserts the new entry into the finalResults list in sorted order.
	  if (finalResults == NULL || totalTokens <= finalResults->numTokens)
	    {
	      newNode->nextJSD = finalResults;
	      finalResults = newNode;
	    } 
	  else
	    {
	      jsdNode * ptr = finalResults;
	      jsdNode * prev = NULL;
	      while (ptr != NULL)
		{
		  if (totalTokens <= ptr->numTokens)
		    {
		      newNode->nextJSD = ptr;
		      prev->nextJSD = newNode;
		      break;
		    }
		  else
		    {
		      prev = ptr;
		      ptr = ptr->nextJSD;
		    }
		}
	      if (ptr == NULL)
		{
		  prev->nextJSD = newNode;		  
		}
	    }
	  //Frees the mean list.
	  freeMeanList(meanList);
	}
    }
  printJSDList(finalResults);
  freeJSD(finalResults);
}
//main.
int main(int argc, char ** argv)
{
  struct stat start;
  //Invalid argv[1] pathname checking.
  if (stat(argv[1], &start) < 0 || !S_ISDIR(start.st_mode))
    {
      printf("ERROR: %s is not a valid directory.\n", argv[1]);
      return 0;
    }

  //Appends / to the directory path in argv[1].
  char * currPath;
  if (argv[1][strlen(argv[1]) - 1] != '/')
    {
      currPath = concatenate(argv[1], "/", "");
    }
  else
    {
      currPath = concatenate(argv[1], "", "");
    }
  
  //Intitializes mutex, shared struct and argument list.
  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  fileNode * sharedStruct = createFileNode("start", 0);
  argList * arguments = createArgList(currPath, sharedStruct, &mutex);
  
  //Begins the file search.
  directoryHandler(arguments);

  //Performs the analysis.
  analysis(sharedStruct);

  //Frees the remaining overhead.
  freeFiles(arguments->shared->nextFile);
  free(sharedStruct);
  free(arguments);
  free(currPath);
  
  return 0;
}
