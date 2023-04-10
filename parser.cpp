#include "parser.hpp"
#include <iostream>
#include <list>
#include <vector>

Parser::Parser(std::string &str) : str(str) {

  this->num_tokens = 0;
  this->tokens = new Command[MAX_COMMANDS];
  int command_counter = 0;

  for (std::size_t i = 0; i < str.length(); i++) {

    // for each command
    std::string curr_substr = "";
    int token_counter = 0;
    char prev_delimiter = '0';
    while (true) {
      char ch = str[i];
      if ((int)i > 0 && str[i - 1] == ' ' && ch == ' ') {
        if (i == str.length() - 1) {
          break;
        }
        i++;
        continue;
      }
      if (ch != ' ' && ch != '<' && ch != '>' && ch != '|' && ch != ';') {
        curr_substr += ch;
      }
      if (ch == ' ' || ch == '<' || ch == '>' || i == str.length() - 1 ||
          ch == '|' || ch == ';') {
        if (token_counter == 0) {
          // ingore whitespace
          if (!curr_substr.empty()) {
            this->tokens[command_counter].exec = curr_substr;
            this->tokens[command_counter].args->push_back(curr_substr);
            this->tokens[command_counter].empty = false;
            token_counter++;
          }
        } else if (prev_delimiter == '<') {
          this->tokens[command_counter].fileIn = curr_substr;
        } else if (prev_delimiter == '>') {
          this->tokens[command_counter].fileOut = curr_substr;
        } else if (ch != '<' && ch != '>') {
          if (!curr_substr.empty()) {
            this->tokens[command_counter].args->push_back(curr_substr);
          }
        }
        if (str[i - 1] != '<' && str[i - 1] != '>' && str[i - 1] != '|' &&
            str[i - 1] != ';') {
          prev_delimiter = ch;
        }
        curr_substr = "";
      }
      // end of the command
      if (ch == '|' || ch == ';' || i == str.length() - 1) {
        prev_delimiter = ch;
        break;
      }
      i++;
    }
    command_counter++;
    this->num_tokens += token_counter;
  }
}

Parser::~Parser() { delete[] this->tokens; }

const int &Parser::getNumTokens() const { return this->num_tokens; }

const Command *Parser::getTokens() const { return this->tokens; }

void Parser::history(std::list<std::string> &history, int command_idx) {

  if (history.size() == 20) {
    history.pop_front();
  }
  history.push_back(this->str);

  std::string first_tok = this->tokens[command_idx].exec;

  if (first_tok == "h") {
    int counter = 0;
    for (const auto &line : history) {
      std::cout << counter + 1 << ") " << line << std::endl;
      counter++;
    }
    return;
  }

  int tok_length = first_tok.length();
  if (tok_length == 3 || tok_length == 4) {
    if (first_tok[0] == 'h' && first_tok[1] == '-') {
      int index = 0;
      if (tok_length == 3) {
        index = (first_tok[2] - '0') - 1;
      } else {
        std::string str_number = "";
        str_number += first_tok[2];
        str_number += first_tok[3];
        const char *ch_number = str_number.c_str();
        index = atoi(ch_number) - 1;
      }
      if (index < 0) {
        std::cout << "Commands in history are listed from number 1 and above "
                  << index << std::endl;
      }
      if (index < (int)history.size()) {
        std::list<std::string>::iterator it = history.begin();
        if (history.size() == 20) {
          index--; // handle pop
        }
        std::advance(it, index);
        std::cout << *it << std::endl;
      }
    }
  }
}

void Parser::split(const std::string &str, char delimiter,
                   std::vector<std::string> &substrings) {

  if (!substrings.empty()) {
    std::cout << "vector of substrings must be empty" << std::endl;
  }
  std::string curr_substr = "";
  for (std::size_t i = 0; i < str.length(); i++) {
    char ch = str[i];
    if (ch != delimiter) {
      curr_substr += ch;
    }
    if (ch == delimiter || i == str.length() - 1) {
      substrings.push_back(curr_substr);
      curr_substr = "";
    }
  }
}