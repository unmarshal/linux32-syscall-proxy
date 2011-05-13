#!/usr/bin/python
import socket,sys

class syscall_request:
   r_eax = "\x00\x00\x00\x00"
   r_ebx = "\x00\x00\x00\x00"
   r_ecx = "\x00\x00\x00\x00"
   r_edx = "\x00\x00\x00\x00"
   r_esi = "\x00\x00\x00\x00"
   r_edi = "\x00\x00\x00\x00"

   stack = ""
   request = ""

   def eax(self, val):
     self.r_eax = val

   def ebx(self, val):
      self.r_ebx = val

   def ecx(self, val):
      self.r_ecx = val

   def edx(self, val):
      self.r_edx = val

   def esi(self, val):
      self.r_esi = val
   
   def edi(self, val):
      self.r_edi = val

   def push(self, val):
      self.stack = self.stack + val

   def get_complete(self):
      self.request = self.r_eax + \
                     self.r_ebx + \
                     self.r_ecx + \
                     self.r_edx + \
                     self.r_esi + \
                     self.r_edi + \
                     self.stack
      return self.request


if __name__ == "__main__":
   sockfd = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

   sockfd.connect(("127.0.0.1", 6666))

   syscall = syscall_request()

   syscall.eax("\x41\x42\x43\x44")
   request = syscall.get_complete()

   sockfd.send(len(request))

   sockfd.close();
