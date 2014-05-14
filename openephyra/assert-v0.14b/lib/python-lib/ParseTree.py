#!/bin/env python
import re
import operator

class ParseTreeFactory(object):
    EMPTY_NODE_MATCHER = re.compile(r"""\(        # open paren
                                     [^()\s]+  # contents
                                     \)        # close paren""", re.VERBOSE)

    LEAF_NODE_MATCHER = re.compile(r"""\(         # open paren
                                       ([^()\s]+) # tag
                                       \s+        # whitespace
                                       ([^()\s]+) # tagged word
                                       \)\s*      # close paren""", re.VERBOSE)

    NODE_START_MATCHER = re.compile(r"""\(        # open paren
                                        ([^()\s]+)# tag
                                        \s+       # whitespace""", re.VERBOSE)

    NODE_END_MATCHER = re.compile(r"""\)\s*       # close paren""", re.VERBOSE)

    def fromString(cls, syntacticParse):
        """Get a ParseTree from a parenthesized string."""
        
        def fromStringHelper(syntacticParse, wordCount):
            leafMatch = cls.LEAF_NODE_MATCHER.match(syntacticParse)
            if leafMatch:
                leaf = ParseTree(leafMatch.group(1), leafMatch.group(2))
                leaf.start = wordCount
                leaf.end = wordCount + 1
                return leaf, syntacticParse[leafMatch.end():], leaf.end

            nodeStartMatch = cls.NODE_START_MATCHER.match(syntacticParse)
            result = ParseTree(nodeStartMatch.group(1))
            result.start = wordCount
            remainder = syntacticParse[nodeStartMatch.end():]
            nodeEndMatch = cls.NODE_END_MATCHER.match(remainder)
            while not nodeEndMatch:
                child, remainder, wordCount = fromStringHelper(remainder, wordCount)
                child.parent = result
                result.children.append(child)
                nodeEndMatch = cls.NODE_END_MATCHER.match(remainder)
            result.end = wordCount
            return result, remainder[nodeEndMatch.end():], wordCount

	# clear all empty nodes
	syntacticParse = cls.EMPTY_NODE_MATCHER.sub(r'', syntacticParse)
	result, remainder, wordCount = fromStringHelper(syntacticParse, 0)
	result.parent = None
	return result
    
    fromString = classmethod(fromString)


class ParseTree(object):
    
    def __init__(self, tag, word=None):
        self.tag = tag
        self.word = word
        self.start = None
        self.end = None
        self.parent = None
        self.children = []
        
    def __repr__(self):
        contents = filter(operator.truth, [self.tag, self.word, ' '.join(map(str, self.children))])
        return '(%s)' % ' '.join(contents)
	
    def __getitem__(self, x):
        if isinstance(x, int):
            start, end = x, x
        if isinstance(x, slice):
            start, end = x.start, x.stop
        else:
            raise TypeError

        for subtree in self.subtrees():
            if subtree.start == start and subtree.end == end:
                yield subtree

    def leaves(self):
        if not self.children:
            yield self

        for child in self.children:
            for leaf in child.leaves():
                yield leaf

    def subtrees(self):
        yield self
        for child in self.children:
            for subtree in child.subtrees():
                yield subtree

if __name__ == "__main__":
	syntacticParse = "(S1 (S (NP (DT Both) (LST) (NNP Coors) (CC and) (NNP Stroh)) (VP (AUX have) (ADVP (RB recently)) (VP (AUX been) (VP (VBG ceding) (NP (NN market) (NN share)) (PP (TO to) (NP (NNP Miller) (CC and) (NNP Anheuser))))))))"
	print "SYNTACTIC PARSE"
	print syntacticParse
	print

	print "TREE"
	tree = ParseTreeFactory.fromString(syntacticParse)
	print tree
	print

	print "SUBTREES"
	for subtree in tree.subtrees():
		print (subtree.start, subtree.end), subtree
	print
	
	print "LEAVES"
	for leaf in tree.leaves():
		print leaf
        print

        print "SUBTREE SLICES"
        leafCount = len(list(tree.leaves()))
        for i in range(leafCount + 1):
            for j in range(i + 1, leafCount + 1):
                print (i, j), list(tree[i:j])
