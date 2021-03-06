#include "ivanp/binner/re_axes.hh"

#include <fstream>
#include <tuple>
#include <vector>
#include <utility>
#include <cctype>

#include "ivanp/error.hh"
#include "ivanp/pcre_wrapper.hh"

#include <iostream>
#define TEST(var) \
  std::cout << "\033[36m" #var "\033[0m = " << var << std::endl;

struct re_axes::store: public
  std::vector<std::pair<ivanp::pcre::regex,axis_type>> { };

re_axes::re_axes(const std::string& filename): _store(new store) {

  std::fstream f(filename);
  if (f.fail()) throw ivanp::error("Error reading file \"",filename,"\"");

  char c; // char buffer
  bool e = false, // expression complete
       l = false, // hit left brace
       u = false, // uniform axis
       comment = false;
  std::string re, num_str;
  std::vector<double> nums;

  const auto push_num = [&](){
    if (num_str.size()!=0) {
      nums.push_back(std::stod(num_str));
      num_str.clear();
    }
  };

  while (f.get(c)) {
    if (comment) {
      if (c=='\n') comment = false;
      continue;
    } else if (c=='#') {
      comment = true;
      continue;
    }
    if (!e) {
      if (!isspace(c)) { e = true; re += c; }
    } else if (!l) {
      if (c!='{') {
        re += c;
      } else {
        l = true;
        while (isspace(re.back())) re.pop_back();
      }
    } else {
      if (u && c==':') throw ivanp::error("more than 1 \':\'");
      if (c!='}') {
        if (isspace(c)) {
          push_num();
        } else if (c==':') {
          if ( ( num_str.size() && nums.size()==0) ||
               (!num_str.size() && nums.size()==1) ) {
            u = true;
            push_num();
          } else throw ivanp::error("out of place \':\'");
        } else num_str += c;
      } else {
        if (u && nums.size()!=3) throw ivanp::error(
          "more than 3 arguments for uniform axis");

        using axis_ptr = ivanp::abstract_axis<double>*;

        if (re.front()!='^') re = '^' + re;
        if (re.back()!='$') re += '$';

        _store->emplace_back( std::piecewise_construct,
          std::tie( re ),
          std::make_tuple( u
            ? static_cast<axis_ptr>(
                new ivanp::uniform_axis<double,true>(
                nums[0], nums[1], nums[2]))
            : static_cast<axis_ptr>(
                new ivanp::container_axis<std::vector<double>,true>(
                std::move(nums)))
          )
        );
        re.clear();

        if (u) nums.clear();

        e = false;
        l = false;
        u = false;
      }
    }
  } // end while c

}

re_axes::~re_axes() { delete _store; }

re_axes::axis_type re_axes::operator[](const std::string& name) const {
  ivanp::pcre::match m;
  for (auto& ra : *_store)
    if (ra.first(m,name.c_str(),name.size())) return ra.second;
  throw ivanp::error("No binning found for \"", name, '\"');
}
