#include "dns-storage.hh"
#include <iomanip>
using namespace std;

bool DNSName::makeRelative(const DNSName& root)
{
  auto us = d_name, them=root.d_name;
  while(!them.empty()) {
    if(us.empty())
      return false;
    if(us.back() == them.back()) {
      us.pop_back();
      them.pop_back();
    }
    else
      return false;
  }
  d_name = us;
  return true;
}

const DNSNode* DNSNode::find(DNSName& name, DNSName& last, const DNSNode** passedZonecut, DNSName* zonecutname) const
{
  cout<<"find called for '"<<name<<"', last is now '"<<last<<"'"<<endl;
  if(!last.empty() && rrsets.count(DNSType::NS)) {
    cout<<"  passed a zonecut, making note of this"<<endl;
    if(passedZonecut)
      *passedZonecut=this;
    if(zonecutname)
      *zonecutname=last;
  }

  if(name.empty()) {
    cout<<"Empty lookup name. ";
    cout<<"Returning node with following types: ";
    for(const auto& c : rrsets)
      cout<<c.first<<" ";
    cout<<endl;
    return this;
  }
  cout<<"Children at this node: ";
  for(const auto& c: children) cout <<"'"<<c.first<<"' ";
  cout<<endl;
  auto iter = children.find(name.back());
  cout<<"Looked for child called '"<<name.back()<<"'"<<endl;
  if(iter == children.end()) {
    cout<<"Found nothing, trying wildcard"<<endl;
    iter = children.find("*");
    if(iter == children.end()) {
      cout<<"Still nothing, returning this node"<<endl;
      return this;
    }
    else {
      cout<<"  Had wildcard match, picking that, matching all labels"<<endl;
      while(name.size() > 1) {
        last.push_front(name.back());
        name.pop_back();
      }
    }
  }
  cout<<"  Had match at this node , continuing to child '"<<iter->first<<"'"<<endl;
  last.push_front(name.back());
  name.pop_back();
  return iter->second.find(name, last, passedZonecut, zonecutname);
}

DNSNode* DNSNode::add(DNSName name) 
{
  cout<<"Add called for '"<<name<<"'"<<endl;
  if(name.size() == 1) {
    cout<<"  Last label, done with add. ";
    if(children.count(name.front()))
      cout<<"Label was present already"<<endl;
    else
      cout<<"Added label as new child"<<endl;
    return &children[name.front()];
  }

  auto back = name.back();
  name.pop_back();
  auto iter = children.find(back);

  if(iter == children.end()) {
    cout<<"Inserting new child for "<<back<<", continuing add there"<<endl;
    return children[back].add(name);
  }
  return iter->second.add(name);
}

DNSName operator+(const DNSName& a, const DNSName& b)
{
  DNSName ret=a;
  for(const auto& l : b.d_name)
    ret.d_name.push_back(l);
  return ret;
}

void DNSNode::visit(std::function<void(const DNSName& name, const DNSNode*)> visitor, DNSName name) const
{
  visitor(name, this);
  for(const auto& c : children)
    c.second.visit(visitor, DNSName{c.first}+name);
}

std::ostream & operator<<(std::ostream &os, const DNSLabel& d)
{
  for(uint8_t a : d.d_s) {
    if(a <= 0x20 || a >= 0x7f) {  // RFC 4343
      os<<'\\'<<setfill('0')<<setw(3)<<(int)a;
      setfill(' '); // setw resets itself
    }
    else if((char)a =='.' || (char)a=='\\')
      os<<"\\"<<(char)a;
    else
      os<<(char)a;
  }
  return os;
}

std::ostream & operator<<(std::ostream &os, const DNSName& d)
{
  for(const auto& l : d.d_name) 
    os<<l<<".";
  return os;
}

void DNSNode::addRRs(std::unique_ptr<RRGen>&&a)
{
  rrsets[a->getType()].add(std::move(a));
}
