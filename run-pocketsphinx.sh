
dir=pocketshpinx
input=where.is.the.louvre.museum.located.wav

for w in wav/*.wav
do
pocketsphinx_continuous \
     -lw   7.0   \
     -topn 16 \
     -fillprob   1e-6  \
     -silprob	0.1 \
     -wip		0.5 \
     -compallsen	yes \
    -beam 1e-80 \
    -maxhmmpf 30000 \
    -infile $w \
    -lm models/lm_giga_64k_nvp_3gram.lm.DMP \
    -hmm models/voxforge_en_sphinx.cd_cont_5000 \
    -dict models/cmu07a.dic \
    -logfn /dev/null

done
