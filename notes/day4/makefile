all:
	pandoc --pdf-engine=xelatex -o output.pdf *.md
html:
	pandoc -t html --css=main.css -o output.html --standalone --mathjax *.md
clean:
	rm -rf *.html tex2pdf.* *.pdf
