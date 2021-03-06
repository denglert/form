#ifndef `TEST'
  #message Use -D TEST=XXX
  #terminate
#else
  #include `NAME_' # `TEST'
#endif
.end

*--#[ CoToTensor :
V p1,p2,q1,q2,nosquare;
Set pp:p1,p2;
CF f;
T Q1,functions;
#$q1 = q1;
#$Q1 = Q1;
L F0 = f(q1,q2) * p1.q1 * p2.q1 * q1.q1 * q1.q2;
#do i={1,...,7,11,...,17,51,61,71,72}
  L F`i' = F0;
#enddo
inexpression F1;
  totensor q1,Q1;
endinexpression;
inexpression F2;
  totensor nosquare,q1,Q1;
endinexpression;
inexpression F3;
  totensor functions,q1,Q1;
endinexpression;
inexpression F4;
  totensor nosquare,functions,q1,Q1;
endinexpression;
inexpression F5;
  totensor !pp,q1,Q1;
endinexpression;
inexpression F6;
  totensor !{p1},q1,Q1;
endinexpression;
inexpression F7;
  totensor nosquare,functions,!pp,q1,Q1;
endinexpression;

inexpression F11;
  totensor $q1,Q1;
endinexpression;
inexpression F12;
  totensor q1,$Q1;
endinexpression;
inexpression F13;
  totensor $q1,$Q1;
endinexpression;
inexpression F14;
  totensor Q1,q1;
endinexpression;
inexpression F15;
  totensor $Q1,q1;
endinexpression;
inexpression F16;
  totensor Q1,$q1;
endinexpression;
inexpression F17;
  totensor $Q1,$q1;
endinexpression;

inexpression F51;
  totensor !{p1,p2},q1,Q1;
endinexpression;

inexpression F61;
  totensor !p1,q1,Q1;
endinexpression;

inexpression F71;
  multiply replace_(q1,nosquare);
  totensor nosquare,functions;
endinexpression;

inexpression F72;
  multiply replace_(q1,nosquare);
  totensor nosquare,functions,nosquare,functions;
endinexpression;

P;
.end
assert succeeded?

assert result("F0") =~ expr("f(q1,q2)*p1.q1*p2.q1*q1.q1*q1.q2")
assert result("F1") =~ expr("f(q1,q2)*Q1(p1,p2,q2,N1_?,N1_?)")
assert result("F2") =~ expr("f(q1,q2)*Q1(p1,p2,q2)*q1.q1")
assert result("F3") =~ expr("f(N1_?,q2)*Q1(p1,p2,q2,N1_?,N2_?,N2_?)")
assert result("F4") =~ expr("f(N1_?,q2)*Q1(p1,p2,q2,N1_?)*q1.q1")
assert result("F5") =~ expr("f(q1,q2)*Q1(q2,N1_?,N1_?)*p1.q1*p2.q1")
assert result("F6") =~ expr("f(q1,q2)*Q1(p2,q2,N1_?,N1_?)*p1.q1")
assert result("F7") =~ expr("f(N1_?,q2)*Q1(q2,N1_?)*p1.q1*p2.q1*q1.q1")

assert result("F1") == result("F11")
assert result("F1") == result("F12")
assert result("F1") == result("F13")
assert result("F1") == result("F14")
assert result("F1") == result("F15")
assert result("F1") == result("F16")
assert result("F1") == result("F17")

assert result("F5") == result("F51")

assert result("F6") == result("F61")

assert result("F71") =~ expr("f(nosquare,q2)*functions(p1,p2,q2,N1_?,N1_?)")
assert result("F72") =~ expr("f(N1_?,q2)*functions(p1,p2,q2,N1_?)*nosquare.nosquare")
*--#] CoToTensor :
*--#[ Issue72 :
* "Setups: PATHVALUE not yet implemented"
#:incdir foo
#:path
* foo/p1.prc
#call p1()
P;
.end
#:incdir
#:path foo/bar
* foo/bar/p1.prc
#call p1()
P;
.end
#prepare write "foo/p1.prc", "#procedure p1()\nL F=12345;\n#endprocedure\n"
#prepare write "foo/bar/p1.prc", "#procedure p1()\nL G=123456;\n#endprocedure\n"
assert succeeded?
assert result("F") =~ expr("12345")
assert result("G") =~ expr("123456")
*--#] Issue72 :
*--#[ Issue84 :
* Set to match with a vector
V p,p1,...,p6;
CF f,g,h;
L F = f(p1,-p1,p2,-p2);
id,all,f(?a,-p?vector_,?b) = f(?a,p,?b)*g(p);
Print +s;
.end
assert succeeded?
assert result("F") =~ expr("
       + f(p1,p1,p2,-p2)*g(p1)
       + f(p1,-p1,p2,p2)*g(p2)
")
*--#] Issue84 :
*--#[ Issue86_1 :
* Feature request: take/drop n-th argument of list
* [with zero-dimensional tables]
CF f;
S x,n,n1,n2;

* Get [1,1]. nargs >= 1.
Table first(f?(x?,?a));
Fill first = f(x);

* Get [last,last]. nargs >= 1.
Table last(f?(?a,x?));
Fill last = f(x);

* Get [2,last]. nargs >= 1.
Table rest(f?(x?,?a));
Fill rest = f(?a);

* Get [1,last-1]. nargs >= 1.
Table most(f?(?a,x?));
Fill most = f(?a);

* Join two functions.
Table join(f?(?a),f?(?b));
Fill join = f(?a,?b);

* Rotate left by n. nargs >= 1.
Table roll(n?int_,f?(?a));
Fill roll =
  + delta_(n)   * f(?a)
  + thetap_(n)  * roll(n-1,join(rest(f(?a)),first(f(?a))))
  + thetap_(-n) * roll(n+1,join(last(f(?a)),most(f(?a))))
;

* Get [1,n]. 1 <= n <= nargs.
Table firstn(n?pos_,f?(?a));
Table firstnimpl(n?pos0_,f?(?a),f?(x?,?b));
Fill firstn = firstnimpl(n,f,f(?a,dum_));
Fill firstnimpl =
  + delta_(n)  * f(?a)
  + thetap_(n) * firstnimpl(n-1,f(?a,x),f(?b))
;

* Get the n-th argument. 1 <= n <= nargs.
Table take(n?pos_,f?(?a));
Fill take = first(roll(n-1,f(?a)));

* Drop the n-th argument. 1 <= n <= nargs.
Table drop(n?pos_,f?(?a));
Fill drop = roll(1-n,most(roll(n,f(?a))));

* Get [n1,n2]. Negative indices count from the end. 1 <= n1 <= n2 <= nargs.
Table slice(n1?!{0,},n2?!{0,},f?(?a));
Fill slice =
  + thetap_(n1) * thetap_(n2) * firstn(n2-n1+1,roll(n1-1,f(?a)))
  + thetap_(n1) * thetap_(-n2) * slice(n1,nargs_(?a)+n2+1,f(?a))
  + thetap_(-n1) * thetap_(n2) * slice(nargs_(?a)+n1+1,n2,f(?a))
  + thetap_(-n1) * thetap_(-n2) * slice(nargs_(?a)+n1+1,nargs_(?a)+n2+1,f(?a))
;

L F0  = f(1,...,9);
L F1  = first(F0);
L F2  = last(F0);
L F3  = rest(F0);
L F4  = most(F0);
L F5  = roll(0,F0);
L F6  = roll(2,F0);
L F7  = roll(-2,F0);
L F8  = firstn(3,F0);
L F9  = take(3,F0);
L F10 = drop(3,F0);
L F11 = slice(3,3,F0);
L F12 = slice(3,6,F0);
L F13 = slice(3,-4,F0);
L F14 = slice(-7,6,F0);
L F15 = slice(-7,-4,F0);

P;
.end
assert succeeded?
assert result("F0")  =~ expr("f(1,2,3,4,5,6,7,8,9)")
assert result("F1")  =~ expr("f(1)")
assert result("F2")  =~ expr("f(9)")
assert result("F3")  =~ expr("f(2,3,4,5,6,7,8,9)")
assert result("F4")  =~ expr("f(1,2,3,4,5,6,7,8)")
assert result("F5")  =~ expr("f(1,2,3,4,5,6,7,8,9)")
assert result("F6")  =~ expr("f(3,4,5,6,7,8,9,1,2)")
assert result("F7")  =~ expr("f(8,9,1,2,3,4,5,6,7)")
assert result("F8")  =~ expr("f(1,2,3)")
assert result("F9")  =~ expr("f(3)")
assert result("F10") =~ expr("f(1,2,4,5,6,7,8,9)")
assert result("F11") =~ expr("f(3)")
assert result("F12") =~ expr("f(3,4,5,6)")
assert result("F13") =~ expr("f(3,4,5,6)")
assert result("F14") =~ expr("f(3,4,5,6)")
assert result("F15") =~ expr("f(3,4,5,6)")
*--#] Issue86_1 :
*--#[ Issue86_2 :
* [with the Translate statement]
CF f;
L F0  = f(1,2,3,4,5,6,7,8,9);

#do i=1,4
  #do j=1,8
    L F`i'`j' = F0;
  #enddo
#enddo

$n1 = 3;
$n2 = 5;
$n3 = 4;

#procedure Test(F,trans)
  inexpression `F'1;
    transform,f,`trans'(3,5);
  endinexpression;
  inexpression `F'2;
    transform,f,`trans'(3,$n2);
  endinexpression;
  inexpression `F'3;
    transform,f,`trans'(3,last-4);
  endinexpression;
  inexpression `F'4;
    transform,f,`trans'(3,last-$n3);
  endinexpression;
  inexpression `F'5;
    transform,f,`trans'($n1,5);
  endinexpression;
  inexpression `F'6;
    transform,f,`trans'($n1,$n2);
  endinexpression;
  inexpression `F'7;
    transform,f,`trans'($n1,last-4);
  endinexpression;
  inexpression `F'8;
    transform,f,`trans'($n1,last-$n3);
  endinexpression;
#endprocedure

#call Test(F1,dropargs)
#call Test(F2,selectargs)
#call Test(F3,addargs)
#call Test(F4,mulargs)
P;
ModuleOption local, $n1,$n2,$n3;
.end
assert succeeded?
assert result("F0")  =~ expr("f(1,2,3,4,5,6,7,8,9)")
assert result("F11") =~ expr("f(1,2,6,7,8,9)")
assert result("F21") =~ expr("f(3,4,5)")
assert result("F31") =~ expr("f(1,2,12,6,7,8,9)")
assert result("F41") =~ expr("f(1,2,60,6,7,8,9)")

assert result("F12") == result("F11")
assert result("F13") == result("F11")
assert result("F14") == result("F11")
assert result("F15") == result("F11")
assert result("F16") == result("F11")
assert result("F17") == result("F11")
assert result("F18") == result("F11")

assert result("F22") == result("F21")
assert result("F23") == result("F21")
assert result("F24") == result("F21")
assert result("F25") == result("F21")
assert result("F26") == result("F21")
assert result("F27") == result("F21")
assert result("F28") == result("F21")

assert result("F32") == result("F31")
assert result("F33") == result("F31")
assert result("F34") == result("F31")
assert result("F35") == result("F31")
assert result("F36") == result("F31")
assert result("F37") == result("F31")
assert result("F38") == result("F31")

assert result("F42") == result("F41")
assert result("F43") == result("F41")
assert result("F44") == result("F41")
assert result("F45") == result("F41")
assert result("F46") == result("F41")
assert result("F47") == result("F41")
assert result("F48") == result("F41")
*--#] Issue86_2 :
*--#[ Issue87 :
* Feature request: (anti)bracketing w.r.t. a set
s a, b, c, d;
set ab: a, b;
L test = (a + b)*(c + d);
b ab;
print +s;
.end
assert succeeded?
assert result("test") =~ expr("
       + b * (
          + d
          + c
          )
       + a * (
          + d
          + c
          )
")
*--#] Issue87 :
