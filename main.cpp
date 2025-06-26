#include <atomic>
#include <iomanip>
#include <thread>
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <vector>
#include <string>
#include <ctime>
#include <cstdlib>

using namespace std;

#ifndef WATCHDOG_FATAL
	#define WATCHDOG_FATAL false
#endif

ofstream _log("stdlog.log");

#define LOG_FATAL 	_log << "[FATAL] " << setw(15) << __func__ << ": "
#define LOG_ERR		_log << "[ERROR] " << setw(15) << __func__ << ": "
#define LOG_WARN 	_log << "[WARN ] " << setw(15) << __func__ << ": "
#define LOG			_log << "[ LOG ] " << setw(15) << __func__ << ": "

struct token;

struct path
{
    token* p;
    int weight;
};

struct token {
    string content;    

	unsigned int weight_sum;
    vector<path> paths; 
};

token* random_weighed(token* tokens, int sums)
{
	if(tokens == nullptr)
	{
		LOG_ERR << "Null token" << endl;
		exit(1);
	}

	if(sums <= 0) // something went quite wrong
	{
		LOG_WARN << "Sum is less than (or equal to) zero" << endl;
		sums = 1;
	}
    
	int rng = (rand() % sums) + 1;
	LOG << "Seeded " << rng << endl;

    for(path curr : tokens->paths )
    {
        if(rng <= curr.weight) {
            return curr.p;
        }
        else rng -= curr.weight;
    }

	// e->paths is empty (this maybe should probably be a while r >= x.weight?)
    return nullptr;
}

string generate_sentence(vector<token> token_list, int size)
{
	if(token_list.size() == 0)
	{
		LOG_WARN << "Empty token_list" << endl;
		return "";
	}

    srand(time(NULL));
    token* next = &token_list[ rand() % token_list.size()];
    string sentence = "";

    while(size--)
    {
        int sums = next->weight_sum;
        next = random_weighed(next, sums);

		if(next == nullptr) 
            return sentence; // SHOULD only happen if next.paths is empty

        sentence += next->content + " ";
    }

    return sentence;
}

int index_of_str_in_path(const string& target, const vector<path>& paths)
{
    int i = 0;
    for(path p : paths)
    {
        if(p.p->content == target) return i;
        i++;
    }

	LOG_WARN << "Failed to find " << target << " in paths" << endl;
    return -1;
}

// TODO: Replace with something from <algorithm>?
vector<string> remove_dupes(vector<string> normal)
{
    vector<string> deduped_vec;

    for(string s : normal)
    {
        for(auto e : deduped_vec)
            if(s == e) goto skip;

        deduped_vec.push_back(s);
        skip: ;
    }

    return deduped_vec;
}

token* str_in_weights(vector<token>& haystack, string needle)
{
    for(token& piece : haystack)
    {
        if(piece.content == needle) return &piece;
    }

	LOG_WARN << "Could not find " << needle << " in haystack sized " << haystack.size() << endl;
    return nullptr;
}

vector<token> tokenize(const vector<string>& s)
{
    vector<token> a;
    vector<string> s_nocopies = remove_dupes(s);
    
    for(string ss : s_nocopies)
    {
        token pre = {
            .content = ss,
            .paths{}
        };

        a.push_back(pre);
    }

	LOG << "Total token count of " << a.size() << endl;

	// Before you ask, no this can't be merged into the loop above
	// because it expects `a` to be fully populated.
	// If merged, you'll get incomplete paths for the first few members
	// of the token list.
	for(token& b : a)
	{
		for(int i = 0; i < s_nocopies.size(); ++i)
		{
			b.paths.push_back(path {
				.p = str_in_weights(a, s_nocopies[i]),
				.weight = 1
			});

			if(b.paths[i].p == nullptr)
				LOG_WARN << "Expect nulls in paths for \"" << b.content << "\"" << endl;
			
		}
	}

    return a;
}

// for std::sort
bool weight_compare(const path &a, const path &b)
{
    return a.weight > b.weight;
}

int erased = 0;
void remove_unlikely_paths(token& b)
{
	int i = 0;
	bool found = false;
	for(auto p : b.paths)
	{
		if(p.weight == 1) {
			found = true;
			break;
		}	
		i++;
	}

	if(found)
	{
//		LOG << "Erased " << b.paths.size() - i << " elements" << endl;
		erased += b.paths.size() - i;
		b.paths.erase(b.paths.begin() + i, b.paths.begin() + b.paths.size());	
	}
}

void weigh(vector<token>& token_list, const vector<string> s)
{
	int index = 0;
    for(token& _token : token_list)
    {
        for(int i = 0; i < s.size(); ++i)
        {
            int c = -1;
            if(i != s.size() - 1 && s[i] == _token.content)
			{
                int c = index_of_str_in_path(s[i + 1], _token.paths);
				if(c < 0)
				{
					LOG_WARN << "Could not find " << s[i] << " in paths for " << _token.content << endl;
					continue;
				}

                _token.paths[c].weight++;
				_token.weight_sum++;
            }
        }
		
		// order paths
		std::sort(_token.paths.begin(), _token.paths.end(), weight_compare);

		remove_unlikely_paths(_token);
	}

	LOG << "Erased " << erased << " low-weight elements" << endl;
}

void dump_weights(const vector<token>& e, string streamname)
{
	ofstream stream(streamname);
    for(token a : e)
    {
        stream << "\"" << a.content << "\"" << endl;
        for(auto x : a.paths)
        {
            stream << "\tS \"" << x.p->content << "\"," << endl;
			stream << "\tW " << x.weight << "" << endl << endl;
        }
    }
}

char as_lower(const char c)
{
    if(c <= 'Z' && c >= 'A') return c + 32;
    return c;
}

bool valid_token_char(char c)
{
	// can use ctype::is_alphanum
	// want this here so i have granular control over token characters
	return  (c >= 'A' && c <= 'Z') ||
			(c >= 'a' && c <= 'z') ||
			(c >= '0' && c <= '9');
}

vector<string> str_to_tok(const string& full_str)
{
    string buff = "";
    vector<string> ret;

	int i = 0; // clean code amirite?

    for( ; i <= full_str.size() ; )
    {
        char c = full_str[i++];

        if(!valid_token_char(c) || i == full_str.size())
		{
            if(buff != "") ret.push_back(buff);
            buff = "";
            continue;
        }

        buff += as_lower(c);
    }

    return ret;
}


void selflearn(vector<token> a, int size, int iterations)
{
    srand(time(NULL));
    token* next = &a[0];
    string fina = "";
    int sizecpy = size;

    while(iterations--)
    {
        size = sizecpy;

        while(size--)
        {
            next = random_weighed(next, next->weight_sum);
            if(next == nullptr) {
                cout << "SIGSEGV" << endl;
                return;
            }

            fina += next->content + " ";
        }
        cout << iterations<< " : " << fina << endl;

        auto tok = str_to_tok(fina);        
        // auto e = tokenize(tok);
        weigh(a, tok);
        
        fina = "";
    }
}

atomic<bool> done = false;
void watchdog()
{
	auto time_ran = 0ms;

	auto maxtime = 5s;

	while(!done.load())
	{
		if(time_ran >= maxtime)
		{
			if(WATCHDOG_FATAL)
			{
				cerr << "Main thread killed by watchdog (took too long)." << endl;
				exit(1);
			}
			else
			{
				maxtime *= 2;
				LOG_ERR << "Watchdog timer reset (watchdog set to nonfatal)" << endl;
			}
		}
		
		time_ran += 250ms;
		this_thread::sleep_for(250ms);
	}
}

string read_file(string path)
{
	std::ifstream in(path.c_str());
	std::ostringstream ss;
	ss << in.rdbuf();

	return ss.str();
}

string memsz(vector<token>& e)
{
	float size = sizeof(token) * e.size() * sizeof(path) * e[0].paths.size();
	string mod = "B";
	if(size != 0)
	{
		if(size / 1024 >= 1) // kb
		{
			mod = "kb";
			size /= 1024;
			if(size / 1024 >= 1) // Mb
			{
				mod = "MB";
				size /= 1024;
				if(size / 1024 >= 1) // GB
				{
					mod = "GB";
					size /= 1024;
				}
			}
		}
	}

	char tmp[7]; // 1023GB
	sprintf(tmp, "%.1f%s", size, mod.c_str());

	return string(tmp);
}

int main(int argc, char** argv)
{
	if(argc <= 1) 
	{
		cerr << "Usage: markov <token_source.txt> [-len x]" << endl;
		return 1;
	}

	argc--; argv++;
	int length = 3;
	string filename = "";
	bool dump = false;

	for(int i = 0; i < argc; ++i)
	{
		string as_string = static_cast<string>(argv[i]);

		if(as_string == "--len" || as_string == "-l")
		{
			length = atoi(argv[i + 1]);
			i++;
		}
		else if(as_string == "--dump" || as_string == "-d")
		{
			dump = true;
		}
		else
		{
			filename = string(argv[i]);
		}
	}

	std::thread w(watchdog);

    auto tok = str_to_tok(read_file(filename));
    auto e = tokenize(tok);
    weigh(e, tok);

    cout << generate_sentence(e, length) << endl; 

	done = true;
	if(dump)
		dump_weights(e, "Dumps.txt");

	LOG << "Total memory usage: " << memsz(e) << endl; 

	w.join();
}
