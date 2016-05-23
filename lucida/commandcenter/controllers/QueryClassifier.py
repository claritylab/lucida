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
from sklearn.metrics import confusion_matrix, f1_score
import os, cPickle

from flask import *

def log(s):
    print s

class QueryClassifier(object):
    # Set of classifiers.
    classifiers = {}
    
    @staticmethod
    def init(TRAIN_OR_LOAD, CLASSIFIER_PIPELINES):
        # Each input type has its own classifier.
        for input_type in CLASSIFIER_PIPELINES:
            # query_classes represents all the possible classification outcomes 
            # and their needed services for a given input type.
            query_classes = CLASSIFIER_PIPELINES[input_type]
            if TRAIN_OR_LOAD == 'train':
                # Check if there is no or only one possible outcomes for the specified input type.
                # If so, there is no need to train any classifier.
                QueryClassifier.classifiers[input_type] = None if len(query_classes) <= 1 else \
                    QueryClassifier.train(input_type, CLASSIFIER_PIPELINES[input_type])
            elif TRAIN_OR_LOAD == 'load':
                if len(query_classes) <= 1:
                    QueryClassifier.classifiers[input_type] = None
                else:
                    try:
                        with open('models/dumped_classifier_' + input_type + '.pkl', 'rb') as fid:
                            print 'Loading model for', input_type
                            QueryClassifier.classifiers[input_type] = cPickle.load(fid)
                    except IOError as e:
                        print e
                        exit(1)
            else:
                raise RuntimeError('app.config["TRAIN_OR_LOAD"] must be either "train" or "load"')
        
        print '@@@@@ Summary of classifiers:'
        print QueryClassifier.classifiers

    
    @staticmethod
    def train(input_type, query_classes):
        # Check if there is no or only one possible outcomes for the specified input type.
        # If so, there is no need to train any classifier.
        if len(query_classes) <= 1:
            return None
        # Build DataFrame by going through all data files.
        data = DataFrame({'text': [], 'class': []})
        for class_name in [query_class[0] for query_class in query_classes]:
            path = 'data/' + class_name + '.txt' # e.g. ../data/IMM_QA.txt
            log('Opening ' + path)
            lines = [line.rstrip('\n') for line in open(path)]
            rows = []
            index = []
            for text in lines:
                if text in index:
                    log('duplicate in ' + path + ": " + text)
                    exit(1)
                rows.append({'text': text, 'class': class_name})
                index.append(text)
            data = data.append(DataFrame(rows, index))
        # Build the pipeline.
        pipeline = Pipeline([
        ('count_vectorizer',   CountVectorizer(ngram_range = (1, 2))),
        #     ('classifier',         PassiveAggressiveClassifier())
        ('classifier',         LinearSVC()) 
        ])
        # Train and k-fold cross-validate.
        data = data.reindex(numpy.random.permutation(data.index)) # introduce randomness
        k_fold = KFold(n=len(data), n_folds=6)
        scores = []
        for train_indices, test_indices in k_fold:
            train_text = data.iloc[train_indices]['text'].values
            train_y = data.iloc[train_indices]['class'].values.astype(str)
            test_text = data.iloc[test_indices]['text'].values
            test_y = data.iloc[test_indices]['class'].values.astype(str)
            pipeline.fit(train_text, train_y)
            predictions = pipeline.predict(test_text)
            score = f1_score(test_y, predictions, pos_label=None if len(query_classes) == 2 else 1,
                            average='weighted')
            scores.append(score)
        log('************************* ' + input_type + ' *************************')
        log('Total documents classified:' + str(len(data)))
        log('Score:' + str(sum(scores) / len(scores)))
        # Save the classifier,
        if not os.path.exists('models'):
            os.makedirs('models')
        with open('models/dumped_classifier_' + input_type + '.pkl', 'wb') as fid:
            print 'Saving model for', input_type
            cPickle.dump(pipeline, fid)
        return pipeline
    
    @staticmethod
    def predict(speech_query, image_query):
        if speech_query:
            if image_query:
                return ['IMM', 'QA']
            else:
                assert 'speech' in QueryClassifier.classifiers \
                    and QueryClassifier.classifiers['speech']
                return QueryClassifier.classifiers['speech'].predict([speech_query])
        else:
            if image_query:
                return ['IMM']
            else:
                raise RuntimeError('Speech and image cannot be both empty')

if __name__ == '__main__':
    log('************************* Scikit Learn *************************')
    app = Flask(__name__)
    app.config.from_object('config')
    QueryClassifier.init(app.config['TRAIN_OR_LOAD'], app.config['CLASSIFIER_PIPELINES'])
    
    print '@@@@@ Result:', QueryClassifier.predict('What is the weight of this computer?', None)



