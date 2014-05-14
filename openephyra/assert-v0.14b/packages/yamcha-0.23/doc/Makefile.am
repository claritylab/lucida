# Process this file with automake to produce Makefile.in

html_HTMLS = yamcha.html 
EXTRA_DIST = $(html_HTMLS) index.html yamcha.css \
		feature.png feature2.png feature3.png \
		feature4.png feature5.png

update-html:
	@for html in $(html_HTMLS); do \
		cmd=`basename $$html .html`.1 && \
		man2html ../man/$$cmd > $$html; \
	done
