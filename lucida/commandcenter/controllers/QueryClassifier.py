import numpy
from pandas import DataFrame
from sklearn.feature_extraction.text import CountVectorizer
from sklearn.linear_model import RidgeClassifier
from sklearn.pipeline import Pipeline
from sklearn.svm import LinearSVC
from sklearn.linear_model import SGDClassifier
from sklearn.linear_model import Perceptron
from sklearn.linear_model import PassiveAggressiveClassifier
from sklearn.naive_bayes import BernoulliNB, MultinomialNB
from sklearn.neighbors import KNeighborsClassifier
from sklearn.neighbors import NearestCentroid
from sklearn.ensemble import RandomForestClassifier
from sklearn.pipeline import Pipeline
from sklearn.cross_validation import KFold
from sklearn.metrics import f1_score
import os, cPickle

from Utilities import log
import Config


# DummyClassifier which only has one possible outcome.
class DummyClassifier(object):
	def __init__(self, query_class_name_in):
		self.query_class_name = query_class_name_in
		
	def predict(self, speech_input):
		return [self.query_class_name]
	
class QueryClassifier(object):
	# Constructor.
	def __init__(self, TRAIN_OR_LOAD, CLASSIFIER_DESCRIPTIONS_IN):
		self.CLASSIFIER_DESCRIPTIONS = CLASSIFIER_DESCRIPTIONS_IN
		self.classifiers = {}
		# Each input type has its own classifier.
		for input_type in self.CLASSIFIER_DESCRIPTIONS:
			# query_classes represents all the possible classification outcomes 
			# and their needed services for a given input type.
			if TRAIN_OR_LOAD == 'train':
				self.classifiers[input_type] = self.train(input_type,
					self.CLASSIFIER_DESCRIPTIONS[input_type])
			elif TRAIN_OR_LOAD == 'load':
				self.classifiers[input_type] = self.load(input_type,
					self.CLASSIFIER_DESCRIPTIONS[input_type])
			else:
				raise RuntimeError(
					'TRAIN_OR_LOAD must be either "train" or "load"')
		log('@@@@@ Summary of classifiers:')
		log(str(self.classifiers))
	
	def train(self, input_type, query_classes):
		log('********************** ' + input_type + ' **********************')
		current_dir = os.path.abspath(os.path.dirname(__file__))
		# If there is no or only one possible outcomes for the input type, 
		# there is no need to train any classifier.
		if len(query_classes) <= 1:
			return DummyClassifier(query_classes.keys()[0])
		# Build DataFrame by going through all data files.
		data = DataFrame({'text': [], 'class': []})
		for query_class_name in query_classes:
			path = current_dir + '/../data/' + query_class_name + '.txt'
			log('Opening ' + path)
			lines = [line.rstrip('\n') for line in open(path)]
			rows = []
			index = []
			for text in lines:
				if text in index:
					log('duplicate in ' + path + ": " + text)
					exit(1)
				rows.append({'text': text, 'class': query_class_name})
				index.append(text)
			data = data.append(DataFrame(rows, index))
		# Build the pipeline.
		pipeline = Pipeline([
		('count_vectorizer',   CountVectorizer(ngram_range = (1, 2))),
		#     ('classifier',         PassiveAggressiveClassifier())
		('classifier',         LinearSVC()) 
		])
		# Train and k-fold cross-validate. Introduce randomness.
		data = data.reindex(numpy.random.permutation(data.index))
		k_fold = KFold(n=len(data), n_folds=6)
		scores = []
		for train_indices, test_indices in k_fold:
			train_text = data.iloc[train_indices]['text'].values
			train_y = data.iloc[train_indices]['class'].values.astype(str)
			test_text = data.iloc[test_indices]['text'].values
			test_y = data.iloc[test_indices]['class'].values.astype(str)
			pipeline.fit(train_text, train_y)
			predictions = pipeline.predict(test_text)
			score = f1_score(test_y, predictions,
							 pos_label=None if len(query_classes) == 2 else 1,
							 average='weighted')
			scores.append(score)
		log('Total documents classified:' + str(len(data)))
		log('Score:' + str(sum(scores) / len(scores)))
		# Save the classifier,
		if not os.path.exists(current_dir + '/../models'):
			os.makedirs(current_dir + '/../models')
		with open(current_dir + '/../models/dumped_classifier_' +
				input_type + '.pkl', 'wb') as fid:
			log('Saving model for ' + input_type)
			cPickle.dump(pipeline, fid)
		return pipeline
	
	def load(self, input_type, query_classes):
		current_dir = os.path.abspath(os.path.dirname(__file__))
		# If there is no or only one possible outcomes for the input type, 
		# there is no need to train any classifier.
		if len(query_classes) <= 1:
			return DummyClassifier(query_classes.keys()[0])
		try:
			with open(current_dir +
				'/../models/dumped_classifier_' + input_type + '.pkl',
				'rb') as fid:
				log('Loading model for ' + input_type)
				return cPickle.load(fid)
		except IOError as e:
			print e
			exit(1)
			
	def predict(self, speech_input, image_input):
		input_type = ''
		if speech_input:
			if image_input:
				input_type = 'text_image'
			else:
				input_type = 'text'
		else:
			if image_input:
				input_type = 'image'
			else:
				raise RuntimeError('Text and image cannot be both empty')      
		# Convert speech_input to a single-element list.
		class_predicted = self.classifiers[input_type].predict([speech_input])
		class_predicted = class_predicted[0] # ndarray to string
		log('Query classified as ' + class_predicted)
		return self.CLASSIFIER_DESCRIPTIONS[input_type][class_predicted]


query_classifier = QueryClassifier(Config.TRAIN_OR_LOAD,
	Config.CLASSIFIER_DESCRIPTIONS)
	