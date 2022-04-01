/*
Copyright (c) 2018-2022, Troy D. Hanson  https://troydhanson.github.io/uthash/
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef UTSTACK_H
#define UTSTACK_H

#define UTSTACK_VERSION 2.3.0

/*
 * This file contains macros to manipulate a singly-linked list as a stack.
 *
 * To use utstack, your structure must have a "next" pointer.
 *
 * ----------------.EXAMPLE -------------------------
 * struct item {
 *      int id;
 *      struct item *next;
 * };
 *
 * struct item *stack = NULL;
 *
 * int main() {
 *      int count;
 *      struct item *tmp;
 *      struct item *item = malloc(sizeof *item);
 *      item->id = 42;
 *      STACK_COUNT(stack, tmp, count); assert(count == 0);
 *      STACK_PUSH(stack, item);
 *      STACK_COUNT(stack, tmp, count); assert(count == 1);
 *      STACK_POP(stack, item);
 *      free(item);
 *      STACK_COUNT(stack, tmp, count); assert(count == 0);
 * }
 * --------------------------------------------------
 */

#define STACK_TOP(head) (head)

#define STACK_EMPTY(head) (!(head))

#define STACK_PUSH(head,add)                                         \
    STACK_PUSH2(head,add,next)

#define STACK_PUSH2(head,add,next)                                   \
do {                                                                 \
  (add)->next = (head);                                              \
  (head) = (add);                                                    \
} while (0)

#define STACK_POP(head,result)                                       \
    STACK_POP2(head,result,next)

#define STACK_POP2(head,result,next)                                 \
do {                                                                 \
  (result) = (head);                                                 \
  (head) = (head)->next;                                             \
} while (0)

#define STACK_COUNT(head,el,counter)                                 \
    STACK_COUNT2(head,el,counter,next)                               \

#define STACK_COUNT2(head,el,counter,next)                           \
do {                                                                 \
  (counter) = 0;                                                     \
  for ((el) = (head); el; (el) = (el)->next) { ++(counter); }        \
} while (0)

#endif /* UTSTACK_H */
