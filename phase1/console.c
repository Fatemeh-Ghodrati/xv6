//
// Console input and output, to the uart.
// Reads are line at a time.
// Implements special input characters:
//   newline -- end of line
//   control-h -- backspace
//   control-u -- kill line
//   control-d -- end of file
//   control-p -- print process list
//

#include <stdarg.h>

#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"

#define BACKSPACE 0x100
#define C(x)  ((x)-'@')  // Control-x
#define MAX_HISTORY 16
#define INPUT_BUF 128
//from RPG_8
struct {
    char buffArr[MAX_HISTORY][INPUT_BUF];
    uint lengthsArr[MAX_HISTORY];
    uint lastCommandIndex;
    int numOfCommandsInMem;
    int currentHistory;
} historyBufferArray;

int history_id = 0;

int sys_history(void){
    uint64 buffer;
    int historyId;
    argint(1, &historyId);
    argaddr(0, &buffer);

    if(historyId >= historyBufferArray.numOfCommandsInMem){
        return -1;
    }
    else if(historyId < 0){
        return -2;
    }
    else if(historyId >= MAX_HISTORY){
        return -3;
    }
    else{
        struct proc *p = myproc();
        copyout(p->pagetable, buffer, historyBufferArray.buffArr[historyId], INPUT_BUF);
    }
    if(historyId == history_id){
        struct proc *p = myproc();
        copyout(p->pagetable, buffer, historyBufferArray.buffArr[historyId], INPUT_BUF);
        return 1;
    }
    return 0;
}


//
// send one character to the uart.
// called by printf(), and to echo input characters,
// but not from write().
//
void
consputc(int c)
{
  if(c == BACKSPACE){
    // if the user typed backspace, overwrite with a space.
    uartputc_sync('\b'); uartputc_sync(' '); uartputc_sync('\b');
  } else {
    uartputc_sync(c);
  }
}

struct {
  struct spinlock lock;
  
  // input
#define INPUT_BUF_SIZE 128
  char buf[INPUT_BUF_SIZE];
  uint r;  // Read index
  uint w;  // Write index
  uint e;  // Edit index
} cons;

//
// user write()s to the console go here.
//
int
consolewrite(int user_src, uint64 src, int n)
{
  int i;
  for(i = 0; i < n; i++){
    char c;
    if(either_copyin(&c, user_src, src+i, 1) == -1)
      break;
    uartputc(c);
  }

  return i;
}

//
// user read()s from the console go here.
// copy (up to) a whole input line to dst.
// user_dist indicates whether dst is a user
// or kernel address.
//
int
consoleread(int user_dst, uint64 dst, int n)
{
  uint target;
  int c;
  char cbuf;

  target = n;
  acquire(&cons.lock);
  while(n > 0){
    // wait until interrupt handler has put some
    // input into cons.buffer.
    while(cons.r == cons.w){
      if(killed(myproc())){
        release(&cons.lock);
        return -1;
      }
      sleep(&cons.r, &cons.lock);
    }

    c = cons.buf[cons.r++ % INPUT_BUF_SIZE];

    if(c == C('D')){  // end-of-file
      if(n < target){
        // Save ^D for next time, to make sure
        // caller gets a 0-byte result.
        cons.r--;
      }
      break;
    }

    // copy the input byte to the user-space buffer.
    cbuf = c;
    if(either_copyout(user_dst, dst, &cbuf, 1) == -1)
      break;

    dst++;
    --n;

    if(c == '\n'){
      // a whole line has arrived, return to
      // the user-level read().
      break;
    }
  }
  release(&cons.lock);

  return target - n;
}

//
// the console input interrupt handler.
// uartintr() calls this for input character.
// do erase/kill processing, append to cons.buf,
// wake up consoleread() if a whole line has arrived.
//

void historyCommand(uint i){
    if(i < 0 || i > historyBufferArray.lastCommandIndex){
        return;
    }
    for (uint m = 0; m < cons.e - cons.r; m++) {
        consputc(BACKSPACE);
    }
    cons.e = cons.w;
    for (int j = 0; j < historyBufferArray.lengthsArr[i]; j++) {
        char c = historyBufferArray.buffArr[i][j];
        consputc(c);
        cons.buf[cons.e++ % INPUT_BUF_SIZE] = c;
    }
}

void
consoleintr(int c)
{
    char buffer[INPUT_BUF];
    acquire(&cons.lock);

  switch(c){
  case C('P'):  // Print process list.
    procdump();
    break;
  case C('U'):  // Kill line.
    while(cons.e != cons.w &&
          cons.buf[(cons.e-1) % INPUT_BUF_SIZE] != '\n'){
      cons.e--;
      consputc(BACKSPACE);
    }
    break;
  case C('H'): // Backspace
  case '\x7f': // Delete key
    if(cons.e != cons.w){
      cons.e--;
      consputc(BACKSPACE);
    }
    break;
  case '\033':
      c = uartgetc();
      if (c == '[') {
          c = uartgetc();
          if (c == 'A') { //Arrow Up
              historyCommand(historyBufferArray.currentHistory);
              if(historyBufferArray.currentHistory != 0) {
                  historyBufferArray.currentHistory--;
              }
              break;
          }
          else if(c == 'B'){ //Arrow Down
              historyCommand(historyBufferArray.currentHistory);
              if(historyBufferArray.currentHistory != historyBufferArray.lastCommandIndex - 1) {
                  historyBufferArray.currentHistory++;
              }
              break;
          }
      }
      break;
  default:
    if(c != 0 && cons.e-cons.r < INPUT_BUF_SIZE){
      c = (c == '\r') ? '\n' : c;

      // echo back to the user.
      consputc(c);

      // store for consumption by consoleread().
      cons.buf[cons.e++ % INPUT_BUF_SIZE] = c;

      if(c == '\n' || c == C('D') || cons.e-cons.r == INPUT_BUF_SIZE){
          consputc(c);

          //copy the command
          for(int i=cons.w, k=0; i < cons.e-1; i++, k++){
              buffer[k] = cons.buf[i % INPUT_BUF];
          }
          buffer[(cons.e-1-cons.w) % INPUT_BUF] = '\0';
          // save the command
          char history_command[] = "history";
          int flag = 0;
          for(int m =0; m < 7; m++){
              if(buffer[m] != history_command[m]){
                  flag = 1;
              }
          }
          if(flag == 0){
            int num[strlen(buffer) - 8];
            for(int m = 0; m < strlen(buffer) - 8; m++){
                num[m] = buffer[m + 8] - '0';
            }
            for(int m =0; m < strlen(buffer) - 8; m++){
                history_id = 10 * history_id + num[m];
            }
          }
          if(buffer[0]!='\0' && flag == 1) {
              int length = strlen(buffer);
              //store
              memmove(historyBufferArray.buffArr[historyBufferArray.lastCommandIndex], buffer, sizeof(char) * length);
              historyBufferArray.buffArr[historyBufferArray.lastCommandIndex][length] = '\0';

              historyBufferArray.lengthsArr[historyBufferArray.lastCommandIndex] = length;
              historyBufferArray.lastCommandIndex = (historyBufferArray.lastCommandIndex + 1) % MAX_HISTORY;
              historyBufferArray.currentHistory = (int)historyBufferArray.lastCommandIndex;

              if (historyBufferArray.numOfCommandsInMem < MAX_HISTORY){
                  historyBufferArray.numOfCommandsInMem++;
              }
          }

        // wake up consoleread() if a whole line (or end-of-file)
        // has arrived.
        cons.w = cons.e;
        wakeup(&cons.r);
      }
    }
    break;
  }
  
  release(&cons.lock);
}

void
consoleinit(void)
{
  initlock(&cons.lock, "cons");

  uartinit();

  // connect read and write system calls
  // to consoleread and consolewrite.
  devsw[CONSOLE].read = consoleread;
  devsw[CONSOLE].write = consolewrite;
}
