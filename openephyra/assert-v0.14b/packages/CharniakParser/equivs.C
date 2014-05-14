
int
equivInt(int x)
{
  int equivpos = eqivPos[x];
  if(equivpos < 0) return x;
  else return equivInt(equivpos);
}

int
puncEquiv(int i, SentRep& sr)
{
  if(i == 0) return i;
  if(scorePunc(sr[i-1].lexeme())) return equivInt(i-1);
  return i;
}
  

void
setEquivInts(SentRep& sr)
{
  int i;
  int len = sr.length();
  equivPos[0] = -1;
  for(i = 1 ; i < len ; i++)
    {
      equivPos[i] = -1;
      int puncequiv = puncEquiv(i,sr);
      if(puncequiv < i) equivPos[i] = puncequiv;
      /*
      int editequiv = editEquiv(i);
      if(editequiv < i)
	{
	  equivPos[i] = editequiv;
	  equivPos[puncequiv] = editequiv;
	}
	*/
    }
}
     


	  
