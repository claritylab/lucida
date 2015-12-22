for bin in surf-fe surf-fd gmm_scoring stem_porter regex_slre crf_tag dnn_asr
do
    for insn in xmm ymm
    do
        echo -n -e "$bin,$insn,"
        find . -name $bin -exec objdump -D {} \; | grep $insn | wc -l
    done
done
