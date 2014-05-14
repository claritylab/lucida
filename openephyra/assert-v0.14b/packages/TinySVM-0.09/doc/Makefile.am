# Process this file with automake to produce Makefile.in

html_HTMLS = svm_learn.html svm_classify.html svm_model.html
EXTRA_DIST = $(html_HTMLS) index.html tinysvm.css

update-html:
	@for html in $(html_HTMLS); do \
		cmd=`basename $$html .html`.1 && \
		man2html ../man/$$cmd > $$html; \
	done
