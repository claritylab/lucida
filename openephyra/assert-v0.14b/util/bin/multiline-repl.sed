:top
/<.*>/{
s/<[^<>]*>//g
t top
}
/</{
	 N
     b top
   }
