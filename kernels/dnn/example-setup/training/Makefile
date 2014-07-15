all:
	@echo "RASR example setup"
	@echo "Possible targets:"
	@echo "  baseline: baseline GMM training (prerequisite for all other recipes)"
	@echo "  vtln    : VTLN estimation and training"
	@echo "  hyrbid  : neural network (NN) training for a hybrid HMM/NN system"
	@echo "  tandem  : neural network (NN) training for a Tandem system"
	@echo ""
	@echo "More details: http://www-i6.informatik.rwth-aachen.de/rwth-asr/manual"
	@echo ""

	@(test -L executables && test -L ../recognition/executables) || \
	    echo "Don't forget to create symlinks to RASR binaries, e.g. \
	    ln -s /home/me/rasr/arch/linux-x86_64-standard executables"

##############################################################################
baseline:
	@echo "####################################################"
	@echo "00. Collect corpus statistics"
	@echo "    result: log/costa.log.gz"
	./00_costa.sh

	@echo "####################################################"
	@echo "01. Extract features"
	@echo "    result: data/mfcc.features.cache"
	./01_feature-extraction.sh

	@echo "####################################################"
	@echo "02. Estimate global covariance normalization matrix"
	@echo "    result: data/total-scatter-normalization.matrix"
	./02_feature-normalization.sh

	@echo "####################################################"
	@echo "03. Estimate initial AM assuming linear segmentation"
	@echo "    result: data/linear-segmentation.mix"
	./03_linear-segmentation.sh

	@echo "####################################################"
	@echo "04. Estimate monophone AM and create first 'good' alignment"
	@echo "    result: data/monophone-alignment.cache"
	./04_monophone-training.sh

	@echo "####################################################"
	@echo "05. Estimate CART for triphone state tying"
	@echo "    result: data/cart.1.tree"
	./05_cart.sh

	@echo "####################################################"
	@echo "07. Estimate LDA transform on 9 consecutive MFCC frames (CART labels = classes)"
	@echo "    result: data/lda-1.matrix"
	./07_lda.sh

	@echo "####################################################"
	@echo "09. Estimate new CART on LDA transformed features"
	@echo "    result: data/cart.2.tree"
	./09_cart.sh

	@echo "####################################################"
	@echo "10. Estimate new LDA on new CART"
	@echo "    result: data/lda-2.matrix"
	./10_lda.sh

	@echo "####################################################"
	@echo "11. Train final AM on the newest CART and LDA and best alignment so far"
	@echo "    result: data/am.lda-2.7-3.mix"
	./11_triphone-lda-2-trainig.sh

	@echo "####################################################"
	@echo "Finished acoustic training. Starting recognition"
	@echo "    result: log/recognition-triphones-lda-2.log.gz"
	cd ../recognition && \
	./01_feature-extraction.sh && ./05_recognition-triphones-lda-2.sh

##############################################################################
vtln:
	@echo "Perform Fast-VTLN warping of the MFCC features."
	@echo "For details see http://www-i6.informatik.rwth-aachen.de/rwth-asr/manual/index.php/VTLN"
	@echo "####################################################"
	@echo "20. Estimate single density GMM"
	@echo "    result: data/am.lda-2.single-gaussian.mix"
	./20_vtln-single-gaussian.sh

	@echo "####################################################"
	@echo "21. For every possible warping factor, perform warping and calculate likelihood"
	@echo "    result: data/warping-factors-map.xml"
	./21_vtln-warp-and-score.sh

	@echo "####################################################"
	@echo "22. Train a GMM as warping factor classifier"
	@echo "    result: data/am.warping-factor-gmm.7-3.mix"
	./22_vtln-train-warping-factor-classifier.sh

	@echo "####################################################"
	@echo "23. Extract warped features"
	@echo "    result: data/base.mfcc-fast-vtln.cache"
	./23_vtln-feature-extraction.sh mfcc-fast-vtln

	@echo "####################################################"
	@echo "24. Reestimate CART and LDA on warped features"
	@echo "    result: data/{cart.4.tree,lda-4.matrix}"
	./24_vtln-reestimate-cart-lda.sh

	@echo "####################################################"
	@echo "25. Train final AM on warped features"
	@echo "    result: data/am.lda-3.7-3.mix"
	./25_vtln-train-triphone.sh

	@echo "VTLN estimation and AM training finished."

	@echo "####################################################"
	@echo "20. Extract warped features for recognition"
	@echo "    result: data/base.mfcc-fast-vtln.recognition.cache"
	cd ../recognition && \
	./20_vtln-feature-extraction.sh

	@echo "####################################################"
	@echo "21. Perform recognition using warped features"
	@echo "    result: log/recognition-triphones-lda-vtln-retrained.log.gz"
	cd ../recognition && \
	./21_vtln-retrained-recognition-triphone-lda.sh

##############################################################################
hybrid:
	@echo "####################################################"
	@echo "30. Estimate global mean and standard deviation for normalization purposes"
	@echo "    result: data/{mean,std}-f32.xml"
	./30_nn-initialization.sh

	@echo "####################################################"
	@echo "31. Train a neural network with 2 hidden layers with 1500 neurons each "
	@echo "    and 501 CART labels as target classes."
	@echo "    result: data/weights-10-*.bin"
	./31_nn-training.sh

	@echo "####################################################"
	@echo "NN training finished. Starting recognition"
	@echo "    result: log/recognition-hybrid.log.gz"
	cd ../recognition && \
	./30_nn-recognition-hybrid.sh

##############################################################################
tandem:
	@echo "####################################################"
	@echo "40. Train a neural network with 3 hidden layers (second is the bottleneck layer)"
	@echo "    result: data/weights-bn-10-*.bin"
	./40_nn-training-tandem.sh

	@echo "####################################################"
	@echo "41. Estimate PCA transform of the bottleneck features and extract dimension-reduced features."
	@echo "    result: data/{pca.matrix,nn.features.cache}"
	./41_nn-feature-extraction-tandem.sh

	@echo "####################################################"
	@echo "42. Estimate Tandem AM on MFCC and bottleneck features."
	@echo "    result: data/am.tandem.7-3.mix"
	./42_nn-gmm-training.sh

	@echo "####################################################"
	@echo "NN training finished."

	@echo "####################################################"
	@echo "40. Extract bottleneck features for recognition"
	@echo "    result: data/nn.features.recognition.cache"
	cd ../recognition && \
	./40_nn-feature-extraction-tandem.sh

	@echo "####################################################"
	@echo "41. Perform recognition with Tandem system"
	@echo "    result: data/recognition-triphones-lda-2-tandem.log.gz"
	cd ../recognition && \
	./41_nn-recognition-tandem.sh

##############################################################################
clean:
	@echo "WARNING: deleting all files in log/ and data/"
	@echo "To continue run 'make SURE=yes clean'"

ifeq ($(SURE),yes)
	rm -vrf data/* log/* ../recognition/log/*
endif
