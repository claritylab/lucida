# Process this file with automake to produce Makefile.in

man_MANS = svm_learn.1 svm_classify.1 svm_model.1
EXTRA_DIST = $(man_MANS)

update-man:
	@for man in $(man_MANS); do \
		cmd=`basename $$man .1` && \
		LANG=C help2man --no-info ../src/$$cmd | \
		perl -pe s/\\bFSF\\b/TinySVM/  | \
		perl -pe 's/(LT|lt)-//g' > $$man; \
	done
