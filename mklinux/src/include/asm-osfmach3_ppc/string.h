#ifndef _PPC_STRING_H_
#define _PPC_STRING_H_

#define __HAVE_ARCH_STRCPY
#define __HAVE_ARCH_STRNCPY
#define __HAVE_ARCH_STRCAT
#define __HAVE_ARCH_STRCMP
#define __HAVE_ARCH_STRLEN
#define __HAVE_ARCH_MEMSET
#define __HAVE_ARCH_MEMCPY
#define __HAVE_ARCH_MEMMOVE
#define __HAVE_ARCH_BCOPY
#define __HAVE_ARCH_MEMCMP

#if 0
extern inline void * memset(void * s,int c,__kernel_size_t count)
{
	char *xs = (char *) s;

	while (count--)
		*xs++ = c;

	return s;
}
#endif

#if 0
#define __HAVE_ARCH_STRSTR
/* Return the first occurrence of NEEDLE in HAYSTACK.  */
extern inline char *
strstr(const char *haystack, const char *needle)
{
  const char *const needle_end = strchr(needle, '\0');
  const char *const haystack_end = strchr(haystack, '\0');
  const size_t needle_len = needle_end - needle;
  const size_t needle_last = needle_len - 1;
  const char *begin;

  if (needle_len == 0)
#ifdef __linux__
    return (char *) haystack;
#else
    return (char *) haystack_end;
#endif
  if ((size_t) (haystack_end - haystack) < needle_len)
    return NULL;

  for (begin = &haystack[needle_last]; begin < haystack_end; ++begin)
    {
      register const char *n = &needle[needle_last];
      register const char *h = begin;

      do
	if (*h != *n)
	  goto loop;		/* continue for loop */
      while (--n >= needle && --h >= haystack);

      return (char *) h;

    loop:;
    }

  return NULL;
}
#endif

#endif
