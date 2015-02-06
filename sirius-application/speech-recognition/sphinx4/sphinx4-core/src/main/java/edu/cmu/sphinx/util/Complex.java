/*
 * Copyright 1999-2002 Carnegie Mellon University.  
 * Portions Copyright 2002 Sun Microsystems, Inc.  
 * Portions Copyright 2002 Mitsubishi Electric Research Laboratories.
 * All Rights Reserved.  Use is subject to license terms.
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 *
 */

package edu.cmu.sphinx.util;

/** Implements complex types and arythmetics */
public class Complex {

    /** The real part of a complex number. */
    private double real;

    /** The imaginary part of a complex number. */
    private double imaginary;


    /** Create a default complex number */
    public Complex() {
        reset();
    }


    /** Create a complex number from a real one */
    public Complex(double real) {
        set(real, 0.0f);
    }


    /** Create a complex number from the real and imaginary parts */
    public Complex(double real, double imaginary) {
        set(real, imaginary);
    }


    /**
     * Returns the real part of this Complex number.
     *
     * @return the real part
     */
    public double getReal() {
        return real;
    }


    /**
     * Returns the imaginary part of this Complex number.
     *
     * @return the imaginary part
     */
    public double getImaginary() {
        return imaginary;
    }


    /** Sets both the real and imaginary parts of this complex number to zero. */
    public void reset() {
        this.real = 0.0f;
        this.imaginary = 0.0f;
    }


    /**
     * Sets the real and imaginary parts of this complex number.
     *
     * @param real      the value of the real part
     * @param imaginary the value of the imaginary part
     */
    public void set(double real, double imaginary) {
        this.real = real;
        this.imaginary = imaginary;
    }


    /**
     * Method to add two complex numbers.
     *
     * @param a the first element to be added
     * @param b the second element to be added
     */
    public void addComplex(Complex a, Complex b) {
        this.real = a.real + b.real;
        this.imaginary = a.imaginary + b.imaginary;
    }


    /**
     * Method to subtract two complex numbers.
     *
     * @param a the element we subtract from
     * @param b the element to be subtracted
     */
    public void subtractComplex(Complex a, Complex b) {
        this.real = a.real - b.real;
        this.imaginary = a.imaginary - b.imaginary;
    }


    /**
     * Method to multiply two complex numbers.
     *
     * @param a the first element to multiply
     * @param b the second element to multiply
     */
    public void multiplyComplex(Complex a, Complex b) {
        this.real = a.real * b.real - a.imaginary * b.imaginary;
        this.imaginary = a.real * b.imaginary + a.imaginary * b.real;
    }


    /**
     * Method to divide two complex numbers. To divide two complexes, we multiply by the complex conjugate of the
     * denominator, thus resulting in a real number in the denominator.
     *
     * @param a the numerator
     * @param b the denominator
     */
    public void divideComplex(Complex a, Complex b) {
        this.real = a.real * b.real + a.imaginary * b.imaginary;
        this.imaginary = a.imaginary * b.real - a.real * b.imaginary;
        this.scaleComplex(this, b.squaredMagnitudeComplex());
    }


    /**
     * Method to scale a complex number by a real one. The input complex number is modified in place.
     *
     * @param a the complex number
     * @param b the real scaling factor
     */
    public void scaleComplex(Complex a, double b) {
        this.real = a.real / b;
        this.imaginary = a.imaginary / b;
    }


    /**
     * Method to compute the squared magnitude of a complex number.
     *
     * @return the squared magnitude of the complex number
     */
    public double squaredMagnitudeComplex() {
        double squaredMag;
        squaredMag = this.real * this.real + this.imaginary * this.imaginary;
        return squaredMag;
    }


    /** Returns this complex number as a string in the format (real, imaginary). */
    @Override
    public String toString() {
        return ("(" + this.real + ", " + this.imaginary + ')');
    }
}
