
dir=pocketshpinx
input=where.is.the.louvre.museum.located.wav

pocketsphinx_continuous \
    -infile wav/$input \
    -lm models/lm_giga_64k_nvp_3gram.lm.DMP \
    -hmm models/voxforge_en_sphinx.cd_cont_5000 \
    -dict models/cmu07a.dic
