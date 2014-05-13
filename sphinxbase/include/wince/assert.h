/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* A bogus <assert.h> for WinCE which sometimes doesn't have it. */
#ifndef __ASSERT_H__
#define __ASSERT_H__
#define assert(x) if (!(x)) (*(int *)0=0);
#endif /* __ASSERT_H__ */
