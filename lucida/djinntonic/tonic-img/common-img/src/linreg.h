#ifndef _LINREG_H_
#define _LINREG_H_

#include <iostream>
#include <math.h>
#include <float.h>

class Point2D {
 public:
  Point2D(double X = 0.0, double Y = 0.0) : x(X), y(Y) {}

  void setPoint(double X, double Y) {
    x = X;
    y = Y;
  }
  void setX(double X) { x = X; }
  void setY(double Y) { y = Y; }

  double getX() const { return x; }
  double getY() const { return y; }

 private:
  double x, y;
};

class LinearRegression {
  // friend ostream& operator<<(ostream& out, LinearRegression& lr);

 public:
  // Constructor using an array of Point2D objects
  // This is also the default constructor
  LinearRegression(Point2D* p = 0, long size = 0);

  // Constructor using arrays of x values and y values
  LinearRegression(double* x, double* y, long size = 0);

  virtual void addXY(const double& x, const double& y);
  void addPoint(const Point2D& p) { addXY(p.getX(), p.getY()); }

  // Must have at least 3 points to calculate
  // standard error of estimate.  Do we have enough data?
  int haveData() const { return (n > 2 ? 1 : 0); }
  long items() const { return n; }

  virtual double getA() const { return a; }
  virtual double getB() const { return b; }

  double getCoefDeterm() const { return coefD; }
  double getCoefCorrel() const { return coefC; }
  double getStdErrorEst() const { return stdError; }
  virtual double estimateY(double x) const { return (a + b * x); }

 protected:
  long n;              // number of data points input so far
  double sumX, sumY;   // sums of x and y
  double sumXsquared,  // sum of x squares
      sumYsquared;     // sum y squares
  double sumXY;        // sum of x*y

  double a, b;   // coefficients of f(x) = a + b*x
  double coefD,  // coefficient of determination
      coefC,     // coefficient of correlation
      stdError;  // standard error of estimate

  void Calculate();  // calculate coefficients
};

#endif  // end of linreg.h
