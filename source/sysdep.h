/* Copyright (C) 2001-2015 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

#ifndef _LINUX_X86_64_SYSDEP_H
#define _LINUX_X86_64_SYSDEP_H 1




//mejbah added for /sysdeps/x86_64/sysdep.h :: start
/* Registers to hold long and pointer.  */
#ifdef __ASSEMBLER__

#define RAX_LP  rax
#define RBP_LP  rbp
#define RBX_LP  rbx
#define RCX_LP  rcx
#define RDI_LP  rdi
#define RDX_LP  rdx
#define RSI_LP  rsi
#define RSP_LP  rsp
#define R8_LP   r8
#define R9_LP   r9
#define R10_LP  r10
#define R11_LP  r11
#define R12_LP  r12
#define R13_LP  r13
#define R14_LP  r14
#define R15_LP  r15

#else 
/* Registers to hold long and pointer.  */
#define RAX_LP  "rax"
#define RBP_LP  "rbp"
#define RBX_LP  "rbx"
#define RCX_LP  "rcx"
#define RDI_LP  "rdi"
#define RDX_LP  "rdx"
#define RSI_LP  "rsi"
#define RSP_LP  "rsp"
#define R8_LP   "r8"
#define R9_LP   "r9"
#define R10_LP  "r10"
#define R11_LP  "r11"
#define R12_LP  "r12"
#define R13_LP  "r13"
#define R14_LP  "r14"
#define R15_LP  "r15"

#endif  /* __ASSEMBLER__ */

//mejbah added for /sysdeps/x86_64/sysdep.h :: end


#ifdef __ASSEMBLER__


/* Linux uses a negative return value to indicate syscall errors,
   unlike most Unices, which use the condition codes' carry flag.

   Since version 2.1 the return value of a system call might be
   negative even if the call succeeded.	 E.g., the `lseek' system call
   might return a large offset.	 Therefore we must not anymore test
   for < 0, but test for a real error by making sure the value in %eax
   is a real error number.  Linus said he will make sure the no syscall
   returns a value in -1 .. -4095 as a valid result so we can savely
   test with -4095.  */

/* We don't want the label for the error handle to be global when we define
   it here.  */
# ifdef PIC
#  define SYSCALL_ERROR_LABEL 0f
# else
#  define SYSCALL_ERROR_LABEL syscall_error
# endif

# undef	PSEUDO
# define PSEUDO(name, syscall_name, args)				      \
  .text;								      \
  ENTRY (name)								      \
    DO_CALL (syscall_name, args);					      \
    cmpq $-4095, %rax;							      \
    jae SYSCALL_ERROR_LABEL

# undef	PSEUDO_END
# define PSEUDO_END(name)						      \
  SYSCALL_ERROR_HANDLER							      \
  END (name)

# undef	PSEUDO_NOERRNO
# define PSEUDO_NOERRNO(name, syscall_name, args) \
  .text;								      \
  ENTRY (name)								      \
    DO_CALL (syscall_name, args)

# undef	PSEUDO_END_NOERRNO
# define PSEUDO_END_NOERRNO(name) \
  END (name)

# define ret_NOERRNO ret

# undef	PSEUDO_ERRVAL
# define PSEUDO_ERRVAL(name, syscall_name, args) \
  .text;								      \
  ENTRY (name)								      \
    DO_CALL (syscall_name, args);					      \
    negq %rax

# undef	PSEUDO_END_ERRVAL
# define PSEUDO_END_ERRVAL(name) \
  END (name)

# define ret_ERRVAL ret

# if defined PIC && defined RTLD_PRIVATE_ERRNO
#  define SYSCALL_SET_ERRNO			\
  lea rtld_errno(%rip), %RCX_LP;		\
  neg %eax;					\
  movl %eax, (%rcx)
# else
//#  if IS_IN (libc)
//#   define SYSCALL_ERROR_ERRNO __libc_errno
//#  else
#   define SYSCALL_ERROR_ERRNO errno
//#  endif
#  define SYSCALL_SET_ERRNO			\
  movq SYSCALL_ERROR_ERRNO@GOTTPOFF(%rip), %rcx;\
  neg %eax;					\
  movl %eax, %fs:(%rcx);
# endif

# ifndef PIC
#  define SYSCALL_ERROR_HANDLER	/* Nothing here; code in sysdep.S is used.  */
# else
#  define SYSCALL_ERROR_HANDLER			\
0:						\
  SYSCALL_SET_ERRNO;				\
  or $-1, %RAX_LP;				\
  ret;
# endif	/* PIC */

/* The Linux/x86-64 kernel expects the system call parameters in
   registers according to the following table:

    syscall number	rax
    arg 1		rdi
    arg 2		rsi
    arg 3		rdx
    arg 4		r10
    arg 5		r8
    arg 6		r9

    The Linux kernel uses and destroys internally these registers:
    return address from
    syscall		rcx
    eflags from syscall	r11

    Normal function call, including calls to the system call stub
    functions in the libc, get the first six parameters passed in
    registers and the seventh parameter and later on the stack.  The
    register use is as follows:

     system call number	in the DO_CALL macro
     arg 1		rdi
     arg 2		rsi
     arg 3		rdx
     arg 4		rcx
     arg 5		r8
     arg 6		r9

    We have to take care that the stack is aligned to 16 bytes.  When
    called the stack is not aligned since the return address has just
    been pushed.


    Syscalls of more than 6 arguments are not supported.  */

# undef	DO_CALL
# define DO_CALL(syscall_name, args)		\
    DOARGS_##args				\
    movl $SYS_ify (syscall_name), %eax;		\
    syscall;

# define DOARGS_0 /* nothing */
# define DOARGS_1 /* nothing */
# define DOARGS_2 /* nothing */
# define DOARGS_3 /* nothing */
# define DOARGS_4 movq %rcx, %r10;
# define DOARGS_5 DOARGS_4
# define DOARGS_6 DOARGS_5

#else	/* !__ASSEMBLER__ */
/* Define a macro which expands inline into the wrapper code for a system
   call.  */
# undef INLINE_SYSCALL
# define INLINE_SYSCALL(name, nr, args...) \
  ({									      \
    unsigned long int resultvar = INTERNAL_SYSCALL (name, , nr, args);	      \
    if (__glibc_unlikely (INTERNAL_SYSCALL_ERROR_P (resultvar, )))	      \
      {									      \
	__set_errno (INTERNAL_SYSCALL_ERRNO (resultvar, ));		      \
	resultvar = (unsigned long int) -1;				      \
      }									      \
    (long int) resultvar; })

/* Define a macro with explicit types for arguments, which expands inline
   into the wrapper code for a system call.  It should be used when size
   of any argument > size of long int.  */
# undef INLINE_SYSCALL_TYPES
# define INLINE_SYSCALL_TYPES(name, nr, args...) \
  ({									      \
    unsigned long int resultvar = INTERNAL_SYSCALL_TYPES (name, , nr, args);  \
    if (__glibc_unlikely (INTERNAL_SYSCALL_ERROR_P (resultvar, )))	      \
      {									      \
	__set_errno (INTERNAL_SYSCALL_ERRNO (resultvar, ));		      \
	resultvar = (unsigned long int) -1;				      \
      }									      \
    (long int) resultvar; })

# undef INTERNAL_SYSCALL_DECL
# define INTERNAL_SYSCALL_DECL(err) do { } while (0)

# define INTERNAL_SYSCALL_NCS(name, err, nr, args...) \
  ({									      \
    unsigned long int resultvar;					      \
    LOAD_ARGS_##nr (args)						      \
    LOAD_REGS_##nr							      \
    asm volatile (							      \
    "syscall\n\t"							      \
    : "=a" (resultvar)							      \
    : "0" (name) ASM_ARGS_##nr : "memory", "cc", "r11", "cx");		      \
    (long int) resultvar; })
# undef INTERNAL_SYSCALL
# define INTERNAL_SYSCALL(name, err, nr, args...) \
  INTERNAL_SYSCALL_NCS (__NR_##name, err, nr, ##args)

# define INTERNAL_SYSCALL_NCS_TYPES(name, err, nr, args...) \
  ({									      \
    unsigned long int resultvar;					      \
    LOAD_ARGS_TYPES_##nr (args)						      \
    LOAD_REGS_TYPES_##nr (args)						      \
    asm volatile (							      \
    "syscall\n\t"							      \
    : "=a" (resultvar)							      \
    : "0" (name) ASM_ARGS_##nr : "memory", "cc", "r11", "cx");		      \
    (long int) resultvar; })
# undef INTERNAL_SYSCALL_TYPES
# define INTERNAL_SYSCALL_TYPES(name, err, nr, args...) \
  INTERNAL_SYSCALL_NCS_TYPES (__NR_##name, err, nr, ##args)

# undef INTERNAL_SYSCALL_ERROR_P
# define INTERNAL_SYSCALL_ERROR_P(val, err) \
  ((unsigned long int) (long int) (val) >= -4095L)

# undef INTERNAL_SYSCALL_ERRNO
# define INTERNAL_SYSCALL_ERRNO(val, err)	(-(val))

/* List of system calls which are supported as vsyscalls.  */
# define HAVE_CLOCK_GETTIME_VSYSCALL    1
# define HAVE_GETTIMEOFDAY_VSYSCALL     1
# define HAVE_GETCPU_VSYSCALL		1

# define LOAD_ARGS_0()
# define LOAD_REGS_0
# define ASM_ARGS_0

# define LOAD_ARGS_TYPES_1(t1, a1)					   \
  t1 __arg1 = (t1) (a1);						   \
  LOAD_ARGS_0 ()
# define LOAD_REGS_TYPES_1(t1, a1)					   \
  register t1 _a1 asm ("rdi") = __arg1;					   \
  LOAD_REGS_0
# define ASM_ARGS_1	ASM_ARGS_0, "r" (_a1)
# define LOAD_ARGS_1(a1)						   \
  LOAD_ARGS_TYPES_1 (long int, a1)
# define LOAD_REGS_1							   \
  LOAD_REGS_TYPES_1 (long int, a1)

# define LOAD_ARGS_TYPES_2(t1, a1, t2, a2)				   \
  t2 __arg2 = (t2) (a2);						   \
  LOAD_ARGS_TYPES_1 (t1, a1)
# define LOAD_REGS_TYPES_2(t1, a1, t2, a2)				   \
  register t2 _a2 asm ("rsi") = __arg2;					   \
  LOAD_REGS_TYPES_1(t1, a1)
# define ASM_ARGS_2	ASM_ARGS_1, "r" (_a2)
# define LOAD_ARGS_2(a1, a2)						   \
  LOAD_ARGS_TYPES_2 (long int, a1, long int, a2)
# define LOAD_REGS_2							   \
  LOAD_REGS_TYPES_2 (long int, a1, long int, a2)

# define LOAD_ARGS_TYPES_3(t1, a1, t2, a2, t3, a3)			   \
  t3 __arg3 = (t3) (a3);						   \
  LOAD_ARGS_TYPES_2 (t1, a1, t2, a2)
# define LOAD_REGS_TYPES_3(t1, a1, t2, a2, t3, a3)			   \
  register t3 _a3 asm ("rdx") = __arg3;					   \
  LOAD_REGS_TYPES_2(t1, a1, t2, a2)
# define ASM_ARGS_3	ASM_ARGS_2, "r" (_a3)
# define LOAD_ARGS_3(a1, a2, a3)					   \
  LOAD_ARGS_TYPES_3 (long int, a1, long int, a2, long int, a3)
# define LOAD_REGS_3							   \
  LOAD_REGS_TYPES_3 (long int, a1, long int, a2, long int, a3)

# define LOAD_ARGS_TYPES_4(t1, a1, t2, a2, t3, a3, t4, a4)		   \
  t4 __arg4 = (t4) (a4);						   \
  LOAD_ARGS_TYPES_3 (t1, a1, t2, a2, t3, a3)
# define LOAD_REGS_TYPES_4(t1, a1, t2, a2, t3, a3, t4, a4)		   \
  register t4 _a4 asm ("r10") = __arg4;					   \
  LOAD_REGS_TYPES_3(t1, a2, t2, a2, t3, a3)
# define ASM_ARGS_4	ASM_ARGS_3, "r" (_a4)
# define LOAD_ARGS_4(a1, a2, a3, a4)					   \
  LOAD_ARGS_TYPES_4 (long int, a1, long int, a2, long int, a3,		   \
		     long int, a4)
# define LOAD_REGS_4							   \
  LOAD_REGS_TYPES_4 (long int, a1, long int, a2, long int, a3,		   \
		     long int, a4)

# define LOAD_ARGS_TYPES_5(t1, a1, t2, a2, t3, a3, t4, a4, t5, a5)	   \
  t5 __arg5 = (t5) (a5);						   \
  LOAD_ARGS_TYPES_4 (t1, a1, t2, a2, t3, a3, t4, a4)
# define LOAD_REGS_TYPES_5(t1, a1, t2, a2, t3, a3, t4, a4, t5, a5)	   \
  register t5 _a5 asm ("r8") = __arg5;					   \
  LOAD_REGS_TYPES_4 (t1, a1, t2, a2, t3, a3, t4, a4)
# define ASM_ARGS_5	ASM_ARGS_4, "r" (_a5)
# define LOAD_ARGS_5(a1, a2, a3, a4, a5)				   \
  LOAD_ARGS_TYPES_5 (long int, a1, long int, a2, long int, a3,		   \
		     long int, a4, long int, a5)
# define LOAD_REGS_5							   \
  LOAD_REGS_TYPES_5 (long int, a1, long int, a2, long int, a3,		   \
		     long int, a4, long int, a5)

# define LOAD_ARGS_TYPES_6(t1, a1, t2, a2, t3, a3, t4, a4, t5, a5, t6, a6) \
  t6 __arg6 = (t6) (a6);						   \
  LOAD_ARGS_TYPES_5 (t1, a1, t2, a2, t3, a3, t4, a4, t5, a5)
# define LOAD_REGS_TYPES_6(t1, a1, t2, a2, t3, a3, t4, a4, t5, a5, t6, a6) \
  register t6 _a6 asm ("r9") = __arg6;					   \
  LOAD_REGS_TYPES_5 (t1, a1, t2, a2, t3, a3, t4, a4, t5, a5)
# define ASM_ARGS_6	ASM_ARGS_5, "r" (_a6)
# define LOAD_ARGS_6(a1, a2, a3, a4, a5, a6)				   \
  LOAD_ARGS_TYPES_6 (long int, a1, long int, a2, long int, a3,		   \
		     long int, a4, long int, a5, long int, a6)
# define LOAD_REGS_6							   \
  LOAD_REGS_TYPES_6 (long int, a1, long int, a2, long int, a3,		   \
		     long int, a4, long int, a5, long int, a6)

#endif	/* __ASSEMBLER__ */


#endif /* linux/x86_64/sysdep.h */
