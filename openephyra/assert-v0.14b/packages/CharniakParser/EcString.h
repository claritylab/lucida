
#ifndef ECSTRING_H
#define ECSTRING_H

#include <string.h>
#include <assert.h>
#include <iostream.h>

class EcString
{
public:
  EcString()
    {
      str_ = new char[1];
      strcpy(str_, "");
    }
  EcString(const char* a) 
    {
      str_ = new char[strlen(a)+1];
      strcpy(str_, a);
    }
  EcString(const EcString& a)
    {
      str_ = new char[strlen(a.str_)+1];
      strcpy(str_, a.str_);
    }
  EcString(const char a) //not in string
    {
      str_ = new char[2];
      str_[0] = a;
      str_[1] = '\0';
    }
  EcString(const EcString& a, int s, int l) //not in string, is in String
    {
      str_ = new char[l+1];
      if(s+1 > a.length())
	{
	  cerr << "Bad EcString creator: " << a << " "
	       << s << " " << l << endl;
	  assert(s+l <= a.length());
	}
      strncpy(str_, (a.str_+s), l);
      str_[l] = '\0';
    }
  ~EcString() { delete str_; }
  operator char*() const { return str_; }
  char* c_str() const { return str_; }
  void  operator=(const char* a)
    {
      delete str_;
      str_ = new char[strlen(a)+1];
      strcpy(str_, a);
    }
  void  operator=(const EcString& a)
    {
      delete str_;
      str_ = new char[strlen(a.str_)+1];
      strcpy(str_, a.str_);
    }
  void  operator+=(const EcString& a)
    {
      int len1 = strlen(str_);
      char temp[256];
      strcpy(temp, str_);
      delete str_;
      strcat(temp, a.str_);
      str_ = new char[strlen(temp)+1];
      strcpy(str_, temp);
    }
  int   operator==(const EcString& a) const
    {
      return(strcmp(str_, a.str_) == 0);
    }
  char  operator[](int i) const { return str_[i]; }
  int   operator<(const EcString& a) const
    {
      return(strcmp(str_, a.str_) < 0);
    }
  int   length() const
    {
      return strlen(str_);
    }
  friend ostream& operator<<(ostream& os, const EcString& a)
    {
      os << a.str_ ;
      return os;
    }
  friend istream&  operator>>(istream& is, EcString& a)
    {
      delete a.str_;
      char temp[256];
      is >> temp;
      a.str_ = new char[strlen(temp)+1];
      strcpy(a.str_, temp);
      return is;
    }
  size_t firstOc(char* lookingfor) { return strcspn(str_, lookingfor); }
  size_t find(char* lookingfor) { return strcspn(str_, lookingfor); }
private:
  char* str_;
};

#endif	/* ! ECSTRING_H */
