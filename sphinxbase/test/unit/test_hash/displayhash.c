/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <hash_table.h>
#include <err.h>

/* Insert -hmmdump, -lm, -svq4svq, -beam, -lminmemory into a hash and display it. */
int
main(int argc, char **argv)
{
    hash_table_t *ht;
    ht = hash_table_new(75, 0);

    if (hash_table_enter(ht, "-hmmdump", (void *)1) != (void *)1) {
        E_FATAL("Insertion of -hmmdump failed\n");
    }

    if (hash_table_enter(ht, "-svq4svq", (void *)1) != (void *)1) {
        E_FATAL("Insertion of -svq4svq failed\n");
    }

    if (hash_table_enter(ht, "-lm", (void *)1) != (void *)1) {
        E_FATAL("Insertion of -lm failed\n");
    }

    if (hash_table_enter(ht, "-beam", (void *)1) != (void *)1) {
        E_FATAL("Insertion of -beam failed\n");
    }

    if (hash_table_enter(ht, "-lminmemory", (void *)1) != (void *)1) {
        E_FATAL("Insertion of -lminmemory failed\n");
    }

    hash_table_display(ht, 1);

    hash_table_free(ht);
    ht = NULL;
    return 0;
}


#if 0
E_INFO("Hash table in the command line\n");
hash_table_display(ht, 1);

E_INFO("After deletion of -lm\n");
hash_table_delete(ht, "-lm");
hash_table_display(ht, 1);

E_INFO("After deletion of -lm\n");

hash_table_delete(ht, "-lm");
hash_table_display(ht, 1);

E_INFO("After deletion of -svq4svq\n");
hash_table_delete(ht, "-svq4svq");
hash_table_display(ht, 1);

E_INFO("After deletion of -beam\n");
hash_table_delete(ht, "-beam");
hash_table_display(ht, 1);
#endif
