
dir=pocketshpinx
input=where.is.the.louvre.museum.located.wav

rm -f scoring-counts.csv log.txt
echo "input,scoring,calls" >> scoring-counts.csv

for w in wav/questions/*.wav
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
     -logfn log.txt

    echo -n "$w" >> scoring-counts.csv
    echo -n "," >> scoring-counts.csv
    total=$(cat log.txt | grep "scoring" | awk -F "," '{print $2}' | awk '{total+=$0} END {printf total}')
    echo -n $total >> scoring-counts.csv
    echo -n "," >> scoring-counts.csv
    cat log.txt | grep "scoring" | wc -l >> scoring-counts.csv
    rm -rf log.txt
done
