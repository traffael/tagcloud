/* This is a word-extractor to be used for generating tag-clouds
 * 
 * author: Raffael Tschui  
 * 
 * NOTE: still bugs inside: incrementing and decrementing the counter
 * does not work as desired.
 */
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <map>
#include <unordered_map> 	// OBSERVE: C++11 feature! if not available, change to map
#include <unordered_set> 	// C++11 feature!
#include <algorithm>
#include <fstream>
#include <locale> 			//note: takes into account local settings for alphabetic character recognition.
							//otherwise use #include <ctype.h>
using namespace std;

//PARAMETER DEFINITION:
#define MIN_RANKING 2 		//minimum number of occurence to appear at the "tag cloud" (-> i.e. to be printed to the console)
//parameters for the combination-reducing-algorithm:
#define MIN_OCCURENCE 2		//how many times the combination must at least have appeared
#define MAX_COMBINATIONS 4 	//how many different combinations a word can have
//#define REL_OCCURENCE 
#define RANKING_OCCURENCE 1 //minimum number of occurencies in the text

//non-desired characters: (incomplete?)
#define PUNCT_SIZE 12 //change this if you add a character down here->
char punct[PUNCT_SIZE]={'.',',','?','!',':',';','(',')','[',']','{','}'};
string TEST1="facebook is a social network and it is a social network which is a website";
string TEST="Cellular respiration is the set of the metabolic \n reactions and processes that take place in the cells of organisms to convert biochemical energy from nutrients into adenosine triphosphate (ATP), and then release waste products.[1] The reactions involved in respiration are catabolic reactions, which break large molecules into smaller ones, releasing energy in the process as they break high-energy bonds. Respiration is one of the key ways a cell gains useful energy to fuel cellular activity. Chemically, cellular respiration is considered an exothermic redox reaction. The overall reaction is broken into many smaller ones when it occurs in the body, most of which are redox reactions themselves. Although technically, cellular respiration is a combustion reaction, it clearly does not resemble one when it occurs in a living cell. This difference is because it occurs in many separate steps. While the overall reaction is a combustion, no single reaction that comprises it is a combustion reaction. Nutrients that are commonly used by animal and plant cells in respiration include sugar, amino acids and fatty acids, and a common oxidizing agent (electron acceptor) is molecular oxygen (O2). The energy stored in ATP can then be used to drive processes requiring energy, including biosynthesis, locomotion or transportation of molecules across cell membranes.";

//declarations
class word;
bool cleanNonAlphabetic(string &input);
string strToLower(string input);
unordered_set<string> init_stopwords(string filename);
//TODO (optional): include information of stopword, to catch groups like "Univeristy of Zurich"

typedef unordered_map<string,word*> wordcollection; //(TODO) eventually better to create a class.
typedef pair<string,word*> mappedword;
typedef map<word*, int> NeighbourType;
bool neighbour_comparer(NeighbourType::value_type &i1, NeighbourType::value_type &i2)
{
return i1.second<i2.second;
}
	

class word
{
	private:
	string mainword; 				//case conserving string --> could be removed if not important.
	int count;     		//how many times did the mainword appear
 	
 	
 	NeighbourType neighbour; 		//the preceding word and a counter
 	
 	unordered_set<word*> referenced;//set of words who have this one as neighbour.
	
	public:
	word(string newword,word* neigh=0)
	:mainword(newword),count(0)
	{
		addEntry(neigh);
	}
	
	void addEntry(word* neigh) 		// !! the counter is increased AND a neighbour added.
	{
		count++;
		if(neigh!=0 && neigh!=this)
		{
			(neighbour[neigh])++;
			neigh->addReference(this);
		}
	}
	
	void addReference(word* ref)
	{
		referenced.insert(ref);
	}
	
	void deleteReference(word* ref)
	{
		referenced.erase(ref);
	}
	
	void deleteNeighbour(word* neigh)
	{
		neighbour.erase(neigh);
	}
	
	string toStr()
	{
		return mainword;
	}	
	
	bool reduce(wordcollection &wc) //sticks together the words who belong together
									// and empties the references
									// return value: true if the word must be deleted from the collection
	{
		NeighbourType::iterator next;
		// delete all neighbours where occurence<MIN_OCCURENCE
		for(NeighbourType::iterator i(neighbour.begin());i!=neighbour.end();i=next)
		{
			if(i->second<MIN_OCCURENCE) //dont consider this link any more.
			{
				(i->first)->deleteReference(this);
				next=neighbour.erase(i);
			}
			else
			{
				i++;
				next=i;
			}
		}
		// reduce number of neighbours to MAX_COMBINATIONS:
		while(neighbour.size()>MAX_COMBINATIONS)
		{
			NeighbourType::iterator i = min_element(neighbour.begin(), neighbour.end(),neighbour_comparer);
			i->first->deleteReference(this);
			neighbour.erase(i);
		}                                 
		
		bool empty(false); //return value
		// create the new word combinations:
		for(NeighbourType::iterator i(neighbour.begin());i!=neighbour.end();i++)
		{
			string newwordstring=(i->first)->toStr() +" "+ this->mainword;//create the combinated string
			word* newword= new word(newwordstring); 
			newword->setCount(i->second); 
			count=count-(i->second); //decrease counter of the individual word by amount of combinations
			if(i->first->decCount(i->second))
			{//decrement counter of used neighbour, and delete it if his counter is 0.
				wc.erase(strToLower(i->first->toStr()));
			}
			
			
			for(unordered_set<word*>::iterator j(referenced.begin());j!=referenced.end();j++)
			{//remove (this) from neighbours of all words referencing to (this).
				(*j)->deleteNeighbour(this);
			}
			referenced.clear();
			
			if(count==0) empty=true;
			wc.insert(mappedword(strToLower(newwordstring),newword));
		}
		
		return empty;
	}

	 int getCount()
	{return count;}
	void setCount(unsigned int c)
	{count=c;}
	bool decCount(unsigned int c)//decrement counter
	{
		count-=c;
		return (count==0); //if true, delete this word from list
	}
	
};	


int main()
{
	wordcollection my_words;
	unordered_set<string> stopwords(init_stopwords("stopwords.txt"));
	
	string tmp,key; //original word; non-case-sensitive word
	
	word* lastword=0; //to assign a neighbour.
	while(cin.good())
	{
		cin>>tmp; //read new word
		if (cleanNonAlphabetic(tmp)) continue;//continues if tmp is empty.
		//check for stop words:
		key=strToLower(tmp);
		if(stopwords.count(key)) continue; //ignore and go to next word
		
		wordcollection::iterator i(my_words.find(key)); //check if already in collection
		
		if(i==my_words.end()) 
		{//create a new word
			word* newword=new word(tmp,lastword);
			my_words.insert(mappedword(key,newword));
			lastword=newword;
		}
		else
		{//reference the last word as neighbour to the found word
			i->second->addEntry(lastword);
			lastword=i->second;
		}
	}
	
	//sorting...
	wordcollection::iterator next;
	for(wordcollection::iterator i(my_words.begin());i!=my_words.end();i=next)
	{
		//cout<<i->first<<',';
		if(i->second->reduce(my_words))
		{
			next=my_words.erase(i);
		}
		else
		{
			i++;
			next=i;
		}
	}
		for(wordcollection::iterator i(my_words.begin());i!=my_words.end();i++)
		{
			if(i->second->getCount()>1)
			cout<<i->second->toStr()<<","<<i->second->getCount()<<endl;
		}
	
	return 0;
}



bool cleanNonAlphabetic(string &input) 			//removes undesired non-alphabetic characters from string
{
	int stringsize=input.size();
	for(int i(0);i<stringsize;)
	{
		
		// !"#$%&'()*+,-./:;<=>?@[\]^_`{|}~
		if(isalpha(input[i])) i++;
		/*else if((input[i]=='-' || input[i]=='/'))
		{
			if(stringsize>2) i++;*/
		else if(isdigit(input[i]) && i==0)//filter out pure numbers (i.e. words that start with a digit)
		{
			input.erase(i,1);
			stringsize=input.size();
		} 	
		else
		{
			bool cont=true;
			for(int j(0); j<PUNCT_SIZE && cont;j++)
			{
				if(input[i]==punct[j])
				{
					input.erase(i,1);
					stringsize=input.size();
					cont=false;
				}
			}
			if(cont) i++; //no interpunctation character found -> dont erase.
		}
		
	}
	return input.empty();
}

string strToLower(string input) 				//makes each letter of a string to lower case
{
	for(unsigned int i(0); i<input.size();i++)
	{
		input[i]=tolower(input[i]);
	}
	return input;
}

unordered_set<string> init_stopwords(string filename)	//initialize stop word set (load from file)
{
	ifstream in(filename.c_str());
	unordered_set<string> stoplist;
	string tmp;
	if (in.fail())
	{
		cout<<"Warning: Stopword-list could not be loaded."<<endl,
		cout<<" ->Useless words will not be ignored."<<endl;
	}
	else
	{
		while(!in.eof())
		{
			in>>tmp;
			stoplist.insert(tmp);
		}
		in.close();
	}
	ifstream in2("paperwords.txt");
	if (in2.fail())
	{
		cout<<"Warning: Paperword-list could not be loaded."<<endl,
		cout<<" ->frequent words used in papers will NOT be ignored."<<endl;
	}
	else
	{
		while(!in2.eof())
		{
			in2>>tmp;
			stoplist.insert(tmp);
		}
		in2.close();
	}
	return stoplist;
}

		
		
