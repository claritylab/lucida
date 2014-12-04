//
//  main.cpp
//  HMMSearch
//
//  Created by vinicius petrucci on 5/21/14.
//  Copyright (c) 2014 univ of michigan. All rights reserved.
//

#include <stdio.h>
#include <sys/time.h> 

#include <iostream>
#include <fstream>

#include <cfloat>

#include <map>
#include <vector>
#include <list>
#include <unordered_map>

int absoluteWordBeamWidth = 300;
int absoluteBeamWidth = 30000;

float relativeBeamWidth = 1E-80;
float relativeWordBeamWidth = 1E-40;

float logRelativeBeamWidth = -1841854.5;
float logRelativeWordBeamWidth = -3.4028235E38;

int max_token_list = 30000;
std::list<long> tokenList;

//std::vector<int> tokenList;
//std::map<int,int> tokenLocation;

std::list<long> resultList;

std::map<long, std::vector<float> > stateData[40];
std::map<long, std::vector<long> > stateSucc;

std::map<long, std::vector<float> > tokenData[40];
std::map<long, std::vector< std::vector<long> >> tokenSucc[40];

std::map<long, std::vector< std::vector<long> >> tokenSucc2;

std::map<long,float> tokenScore[40];
std::map<long,float> stateProb[40];

std::map<long,bool> stateEmit[40];

std::map<long,bool> tokenWord[40];
std::map<long,bool> tokenFinal[40];

std::pair <long,float> bestTokenScore[40];

//std::map <long,long> tokenState[40];
std::map <long,long> tokenState;

std::map <long,long> stateToken[40];

void read_data(const char *filename) {

    std::fstream myfile(filename, std::ios_base::in);

    long frame_id, arc_len, state_id, token_id, token_is_final, token_is_word;
    float token_score;

    long next_token_id, next_state_id, next_state_is_emitting, next_token_is_final, next_state_is_word;
    float arc_prob, next_token_score;

    //pw_in.printf("%d %d %d %f %d %d\n", currentFrameNumber, arcs.length, token.hashCode(), token.getScore(), (token.isFinal()) ? 1 : 0, (state instanceof WordSearchState) ? 1:0);

    while (myfile >> frame_id >> arc_len >> state_id >> token_id >> token_score >> token_is_final >> token_is_word) {

        //   std::cout << frame_id << " " << arc_len << " "<< token_id << " "<< token_score << " "<< token_is_final << " "<< token_is_word << std::endl;

        std::vector<float> data;

        data.push_back(token_score);
        data.push_back(token_is_final);
        data.push_back(token_is_word);
        data.push_back(arc_len);

        tokenData[frame_id][token_id] = data;
        tokenScore[frame_id][token_id] = token_score;

        tokenState[token_id] = state_id;
        //      tokenState[frame_id][token_id] = state_id;

        stateToken[frame_id][state_id] = token_id;

        tokenWord[frame_id][token_id] = (token_is_word) ? true:false;
        tokenFinal[frame_id][token_id] = (token_is_final) ? true:false;

        // tokenLocation[token_id] = -1;

        // std::cout << bestTokenScore[frame_id].first << std::endl;

        if (bestTokenScore[frame_id].first == 0) {
            bestTokenScore[frame_id].first = token_id;
            bestTokenScore[frame_id].second = token_score;
        } else {
            if (bestTokenScore[frame_id].second < token_score) {
                bestTokenScore[frame_id].first = token_id;
                bestTokenScore[frame_id].second = token_score;
            }
        }

        if (arc_len > 0) {

            std::vector< std::vector<long> > succListData;

            for (int i = 0; i < arc_len; i++) {

                std::vector<long> succData;

                //pw_in.printf("%d %d %d %f\n", newToken.hashCode(), nextState.hashCode(), (nextState.isEmitting()) ? 1:0, arc.getProbability());
                myfile >> next_token_id >> next_token_score >> next_state_id >> next_state_is_emitting >> arc_prob >> next_token_is_final >> next_state_is_word;

                //   std::cout << next_token_id << " " << next_token_score << " "<< next_state_id << " "<< next_state_is_emitting << " "<< arc_prob << std::endl;

                succData.push_back(next_token_id);
                //succData.push_back(next_token_score);
                succData.push_back(next_state_id);
                succData.push_back(next_state_is_emitting);
                // succData.push_back(arc_prob);

                succListData.push_back(succData);

                tokenScore[frame_id][next_token_id] = next_token_score;
                stateProb[frame_id][next_state_id] = arc_prob;

                stateEmit[frame_id][next_state_id] = (next_state_is_emitting) ? true:false;

                tokenWord[frame_id][next_token_id] = (next_state_is_word) ? true:false;
                tokenFinal[frame_id][next_token_id] = (next_token_is_final) ? true:false;

                //      tokenLocation[next_token_id] = -1;

                stateSucc[state_id].push_back(next_state_id);

                tokenState[next_token_id] = next_state_id;
                //                tokenState[frame_id][next_token_id] = next_state_id;

                stateToken[frame_id][next_state_id] = next_token_id;

            }

            tokenSucc[frame_id][token_id] = succListData;
            tokenSucc2[token_id] = succListData;

        }
    }
}


void write_data_out(const char *filename) {

    std::fstream myfile(filename, std::ios_base::out);

    std::cout << "size of resultList: " << resultList.size() << std::endl;

    for (std::list<long>::iterator it=resultList.begin(); it!=resultList.end(); ++it)
        myfile << *it << " ";

    /* for(std::list<int>::size_type i = 0; i != resultList.size(); i++) {
       myfile << resultList[i] << " ";
       }*/

    myfile << std::endl;
    myfile.close();
}

int main(int argc, const char * argv[])
{
    struct timeval tim;

    //tokenList.resize(10000);
    //tokenSize = 0;

    std::cout << "Read input data" << std::endl;

    gettimeofday(&tim, NULL);
    double t1 = tim.tv_sec+(tim.tv_usec/1000000.0);

    // read_data("/Users/vinicius/umvoice/hmm_data_in.txt");
    read_data("/Users/vinicius/umvoice/hmm_data_frame10.txt");

    gettimeofday(&tim, NULL);
    double t2 = tim.tv_sec+(tim.tv_usec/1000000.0);

    printf("Done. %.6lf seconds elapsed\n", t2-t1);

    long bestToken = bestTokenScore[0].first;
    float bestScore = bestTokenScore[0].second;

    // init token list
    tokenList.push_back(bestToken);

    std::cout << "Best token: " << bestToken << " with best score: " << bestScore << std::endl;

    // loop over each frame
    //    for (int i = 0; i < 40; i++) {
    for (int i = 0; i < 10; i++) {

        std::map<long, long> bestTokenMap;
        std::list<long> oldTokenList (tokenList);

        tokenList.clear();

        // compute thresholds
        // float bestToken = bestTokenScore[i].first;
        float bestScore = bestTokenScore[i].second;

        float threshold = bestScore + logRelativeBeamWidth;
        float wordThreshold = bestScore + logRelativeWordBeamWidth;

        resultList.clear();

        std::cout << "# of tokens at frame " << i << " " << oldTokenList.size() << std::endl;

        for (std::list<long>::iterator it=oldTokenList.begin(); it!=oldTokenList.end(); ++it) {

            long token_id = *it;

            float token_score = tokenScore[i][token_id];

            if (i > 0 && token_score < threshold) {
                continue;
            }
            if (i > 0 && (tokenWord[i][token_id] && token_score < wordThreshold)) {
                continue;
            }

            if (tokenFinal[i][token_id]) {
                resultList.push_back(token_id);
                std::cout << "token is final: " << token_id << std::endl;
            }

            //std::vector<std::vector<long>> succData = tokenSucc[i][token_id];
            //std::vector<std::vector<long>> succData = tokenSucc2[token_id];

            //if (data.size() == 0)
            //continue;

            long state = tokenState[token_id];

            // grow branches: loop over next states
            //for (int j = 0; j < succData.size(); j++) {
            for (int j = 0; j < stateSucc[state].size(); j++) {

                //  long next_token_id = succData[j][0];
                //float next_token_score = tokenScore[i][next_token_id];
                /// long next_state_id = succData[j][1];
                //long next_state_is_emitting = succData[j][2];

                long next_state_id = stateSucc[state][j];
                float arc_prob = stateProb[i][next_state_id];
                long next_token_id = stateToken[i][next_state_id];

                float entryScore = token_score + arc_prob;
                bool firstToken = (bestTokenMap.find(next_state_id) == bestTokenMap.end());
                long bestT = bestTokenMap[next_state_id];

                //if (!firstToken)
                //    std::cout << "firstToken? " << firstToken << " BestToken: " << bestT << std::endl;

                if (firstToken || (tokenScore[i][bestT] <= entryScore)) {
                    bestTokenMap[next_state_id] = next_token_id;
                }

                if (!stateEmit[i][next_state_id]) {
                    continue;
                }

                if (firstToken) {
                    tokenList.push_back(next_token_id);
                } else if (tokenScore[i][bestT] < entryScore) {
                }
            }
        }

        // prune poor branches
        if (tokenList.size() > absoluteBeamWidth) {
            tokenList.sort(std::greater<int>());
            std::list<long>::iterator range_begin = tokenList.begin();
            std::advance(range_begin, absoluteBeamWidth);
            tokenList.erase(range_begin, tokenList.end());
        }

        }

        std::cout << "Done." << std::endl;

        write_data_out("hmm_out.txt");

        return 0;
    }
