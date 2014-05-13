/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <hash_table.h>
#include <err.h>

/* Explore a more complicated case for deletion */
int
main(int argc, char **argv)
{
    hash_table_t *ht;
    void *val;
    char *bkey = "key";

    if (argc != 2) {
        printf("deletehash <key>\n");
        exit(-1);
    }

    ht = hash_table_new(75, 0);

    if (hash_table_enter(ht, "-hmmdump", (void *)1) != (void *)1) {
        E_FATAL("Insertion of -hmmdump failed\n");
    }

    if (hash_table_enter(ht, "-svq4svq", (void *)2) != (void *)2) {
        E_FATAL("Insertion of -svq4svq failed\n");
    }

    if (hash_table_enter(ht, "-outlatdir", (void *)3) != (void *)3) {
        E_FATAL("Insertion of -svq4svq failed\n");
    }

    if (hash_table_enter(ht, "-lm", (void *)4) != (void *)4) {
        E_FATAL("Insertion of -lm failed\n");
    }

    if (hash_table_enter(ht, "-beam", (void *)5) != (void *)5) {
        E_FATAL("Insertion of -beam failed\n");
    }

    if (hash_table_enter(ht, "-lminmemory", (void *)6) != (void *)6) {
        E_FATAL("Insertion of -lminmemory failed\n");
    }

    if (hash_table_enter(ht, "-subvq", (void *)7) != (void *)7) {
        E_FATAL("Insertion of -outlatdir failed\n");
    }

    if (hash_table_enter(ht, "-bla", (void *)8) != (void *)8) {
        E_FATAL("Insertion of -bla failed\n");
    }

    /*  hash_table_display(ht,1); */
    if (hash_table_delete(ht, argv[1]) == NULL) {
        E_INFOCONT("Failed as expected\n");
        return 0;
    }
    else {
        hash_table_display(ht, 1);
    }

    /* Test emptying */
    hash_table_empty(ht);
    if (hash_table_lookup(ht, "-beam", &val) == 0) {
        E_FATAL("Emptying hash table failed\n");
    }

    hash_table_free(ht);
    ht = NULL;

    /* Test bkey */
    ht = hash_table_new(75, HASH_CASE_YES);

    if (hash_table_enter_bkey(ht, bkey, 3, (void *)1) != (void *)1) {
        E_FATAL("Insertion of bkey failed\n");
    }
    if (hash_table_lookup_bkey(ht, bkey, 3, &val) != 0) {
        E_FATAL("Lookup failed\n");
    }
    if (hash_table_delete_bkey(ht, bkey, 3) == NULL) {
        E_FATAL("Delete bkey failed\n");
    }
    if (hash_table_lookup_bkey(ht, bkey, 3, &val) != -1) {
        E_FATAL("Second bkey lookup failed\n");
    }
    hash_table_empty(ht);
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
