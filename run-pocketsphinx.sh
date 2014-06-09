
dir=pocketshpinx
input=where.is.the.louvre.museum.located.wav

for w in wav/*.wav
do
pocketsphinx_continuous \
    -infile $w \
    -lm models/lm_giga_64k_nvp_3gram.lm.DMP \
    -hmm models/voxforge_en_sphinx.cd_cont_5000 \
    -dict models/cmu07a.dic

done
