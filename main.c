#include <stdio.h>
#include <stdlib.h>

#define STACK_MAX 256
/*
This project seeks to build a very simple garbage collector.
It was built by following 
https://journal.stuffwithstuff.com/2013/12/08/babys-first-garbage-collector/
*/

// Create the different types in the object
typedef enum{
    OBJ_INT,
    OBJ_PAIR
} ObjectType;

// The Object
typedef struct sObject{
    struct sObject* next;// next object in the list of Objects

    unsigned char marked; // is this marked for deletion

    ObjectType type;
    union{
        int value; // Int Object

        struct { // Pair Object
            struct sObject* head;
            struct sObject* tail;
        };
    };
} Object;

//Creates the VM
typedef struct {
    Object* firstObject; // First Object in the List
    Object* stack[STACK_MAX];
    int stackSize;
    int numObjects;
    int maxObjects;
} VM;

void assert(int condition, const char* message);

// Creates and Inits the VM
VM* newVM();

void freeVM(VM *vm);

// Pushes to VM's Stack
void push(VM* vm, Object* value);

// Pops and object from the VM's Stack
Object* pop(VM* vm);

// Creates an Object
Object* newObject(VM* vm, ObjectType type);

// Pushes an Int to the Object
void pushInt(VM* vm, int intValue);

// Pushes a Pair to the Object
Object* pushPair(VM* vm);

// Marks objects for deletion
void mark(Object* object);

// Marks all Objects for Deletion
void markAll(VM* vm);

// FInds Unmarked Object so they cna be deleted
void sweep(VM* vm);

// Calls the Garbage Collector
void gc(VM* vm);


void test1() {
  printf("Test 1: Objects on stack are preserved.\n");
  VM* vm = newVM();
  pushInt(vm, 1);
  pushInt(vm, 2);

  gc(vm);
  assert(vm->numObjects == 2, "Should have preserved objects.");
  freeVM(vm);
}

void test2() {
  printf("Test 2: Unreached objects are collected.\n");
  VM* vm = newVM();
  pushInt(vm, 1);
  pushInt(vm, 2);
  pop(vm);
  pop(vm);

  gc(vm);
  assert(vm->numObjects == 0, "Should have collected objects.");
  freeVM(vm);
}

void test3() {
  printf("Test 3: Reach nested objects.\n");
  VM* vm = newVM();
  pushInt(vm, 1);
  pushInt(vm, 2);
  pushPair(vm);
  pushInt(vm, 3);
  pushInt(vm, 4);
  pushPair(vm);
  pushPair(vm);

  gc(vm);
  assert(vm->numObjects == 7, "Should have reached objects.");
  freeVM(vm);
}

void test4() {
  printf("Test 4: Handle cycles.\n");
  VM* vm = newVM();
  pushInt(vm, 1);
  pushInt(vm, 2);
  Object* a = pushPair(vm);
  pushInt(vm, 3);
  pushInt(vm, 4);
  Object* b = pushPair(vm);

  /* Set up a cycle, and also make 2 and 4 unreachable and collectible. */
  a->tail = b;
  b->tail = a;

  gc(vm);
  assert(vm->numObjects == 4, "Should have collected objects.");
  freeVM(vm);
}

void perfTest() {
  printf("Performance Test.\n");
  VM* vm = newVM();

  for (int i = 0; i < 1000; i++) {
    for (int j = 0; j < 20; j++) {
      pushInt(vm, i);
    }

    for (int k = 0; k < 20; k++) {
      pop(vm);
    }
  }
  freeVM(vm);
}

int main(int argc, const char * argv[]) {
  test1();
  test2();
  test3();
  test4();
  perfTest();

  return 0;
}

void assert(int condition, const char* message) {
  if (!condition) {
    printf("%s\n", message);
    exit(1);
  }
}

// VM FUNCTIONS
// Creates and Inits the VM
VM* newVM(){
    VM* vm = malloc(sizeof(VM));
    vm->stackSize = 0;
    vm->firstObject = NULL;
    vm->numObjects = 0;
    vm->maxObjects = 10;
    return vm;
}

void freeVM(VM *vm){
    vm->stackSize = 0;
    gc(vm);
    free(vm);
}

// Pushes to VM's Stack
void push(VM* vm, Object* value){
    assert(vm->stackSize < STACK_MAX, "Stack Overflow"); // Stack Overflow
    vm->stack[vm->stackSize++] = value;
}

// Pops and object from the VM's Stack
Object* pop(VM* vm){
    assert(vm->stackSize > 0, "Stack Underflow"); // Stack Underflow
    return vm->stack[--vm->stackSize];
}

// OBJECT FUNCTIONS
// Creates an Object
Object* newObject(VM* vm, ObjectType type){
    if(vm->numObjects == vm->maxObjects) gc(vm); // calls GC if number of objects is exceeded

    Object* object = malloc(sizeof(Object));
    object->type = type;
    object->marked = 0;
    object->next = vm->firstObject;

    vm->firstObject = object;
    vm->numObjects++;

    return object;
}

// Pushes an Int to the Object
void pushInt(VM* vm, int intValue){
    Object* object = newObject(vm, OBJ_INT);
    object->value = intValue;
    push(vm, object);
}

// Pushes a Pair to the Object
Object* pushPair(VM* vm){
    Object* object = newObject(vm, OBJ_PAIR);
    object->tail = pop(vm);
    object->head = pop(vm);

    push(vm, object);
    return object;
}

//MARK FUNCTIONS

// Marks objects for deletion
void mark(Object* object){
    if(object->marked) return; // If this object is marked return 

    object->marked = 1; // otherwise mark this object

    if(object->type == OBJ_PAIR){
        mark(object->head);
        mark(object->tail);
    }
}

// marks all objects
void markAll(VM* vm){
    for(int i = 0; i < vm->stackSize; i++){
        mark(vm->stack[i]);
    }
}

// FInds Unmarked Object so they cna be deleted
void sweep(VM* vm){
    Object** object = &vm->firstObject;
    while(*object){
        if(!(*object)->marked){
            Object* unreached = *object;

            *object = unreached->next;
            free(unreached);
            vm->numObjects--;
        }
        else{
            (*object)->marked = 0;
            object = &(*object)->next;
        }
    }
}

// Garbage Collector
void gc(VM* vm){
    int numObjects = vm->numObjects;

    markAll(vm);
    sweep(vm);

    vm->maxObjects = vm->numObjects * 2;
}