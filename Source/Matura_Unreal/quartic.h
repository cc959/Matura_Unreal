// slightly modified from https://github.com/sasamil/Quartic
#pragma once

#include <complex>

inline std::vector<double> solve_quartic(double a, double b, double c, double d, double e)
{
    const double pi = 3.141592653589793238463L;
    const double M_2pi = 2*pi;
    const double eps = 1e-12;


    b /= a, c /= a, d /= a, e /= a;
    a = b, b = c, c = d, d = e;

    double a3 = -b;
    double b3 =  a*c -4.*d;
    double c3 = -a*a*d - c*c + 4.*b*d;

    auto solveP3 = [pi, M_2pi, eps](double *x,double a,double b,double c) -> unsigned int {
        double a2 = a*a;
        double q  = (a2 - 3*b)/9;
        double r  = (a*(2*a2-9*b) + 27*c)/54;
        double r2 = r*r;
        double q3 = q*q*q;
        double A,B;
        if(r2<q3)
        {
            double t=r/sqrt(q3);
            if( t<-1) t=-1;
            if( t> 1) t= 1;
            t=acos(t);
            a/=3; q=-2*sqrt(q);
            x[0]=q*cos(t/3)-a;
            x[1]=q*cos((t+M_2pi)/3)-a;
            x[2]=q*cos((t-M_2pi)/3)-a;
            return 3;
        }
        else
        {
            A =-pow(fabs(r)+sqrt(r2-q3),1./3);
            if( r<0 ) A=-A;
            B = (0==A ? 0 : q/A);

            a/=3;
            x[0] =(A+B)-a;
            x[1] =-0.5*(A+B)-a;
            x[2] = 0.5*sqrt(3.)*(A-B);
            if(fabs(x[2])<eps) { x[2]=x[1]; return 2; }

            return 1;
        }
    };

    // cubic resolvent
    // y^3 − b*y^2 + (ac−4d)*y − a^2*d−c^2+4*b*d = 0
    double x3[3];
    unsigned int iZeroes = solveP3(x3, a3, b3, c3);

    double q1, q2, p1, p2, D, sqD, y;

    y = x3[0];
    // THE ESSENCE - choosing Y with maximal absolute value !
    if(iZeroes != 1)
    {
        if(fabs(x3[1]) > fabs(y)) y = x3[1];
        if(fabs(x3[2]) > fabs(y)) y = x3[2];
    }

    // h1+h2 = y && h1*h2 = d  <=>  h^2 -y*h + d = 0    (h === q)

    D = y*y - 4*d;
    if(fabs(D) < eps) //in other words - D==0
    {
        q1 = q2 = y * 0.5;
        // g1+g2 = a && g1+g2 = b-y   <=>   g^2 - a*g + b-y = 0    (p === g)
        D = a*a - 4*(b-y);
        if(fabs(D) < eps) //in other words - D==0
            p1 = p2 = a * 0.5;

        else
        {
            sqD = sqrt(D);
            p1 = (a + sqD) * 0.5;
            p2 = (a - sqD) * 0.5;
        }
    }
    else
    {
        sqD = sqrt(D);
        q1 = (y + sqD) * 0.5;
        q2 = (y - sqD) * 0.5;
        // g1+g2 = a && g1*h2 + g2*h1 = c       ( && g === p )  Krammer
        p1 = (a*q1-c)/(q1-q2);
        p2 = (c-a*q2)/(q1-q2);
    }

    std::vector<std::complex<double>> complex_roots(4);

    // solving quadratic eq. - x^2 + p1*x + q1 = 0
    D = p1*p1 - 4*q1;
    if(D < 0.0)
    {
        complex_roots[0].real(-p1 * 0.5 );
        complex_roots[0].imag(sqrt(-D) * 0.5 );
        complex_roots[1] = std::conj(complex_roots[0]);
    }
    else
    {
        sqD = sqrt(D);
        complex_roots[0].real((-p1 + sqD) * 0.5 );
        complex_roots[1].real((-p1 - sqD) * 0.5 );
    }

    // solving quadratic eq. - x^2 + p2*x + q2 = 0
    D = p2*p2 - 4*q2;
    if(D < 0.0)
    {
        complex_roots[2].real(-p2 * 0.5 );
        complex_roots[2].imag(sqrt(-D) * 0.5 );
        complex_roots[3] = std::conj(complex_roots[2]);
    }
    else
    {
        sqD = sqrt(D);
        complex_roots[2].real((-p2 + sqD) * 0.5 );
        complex_roots[3].real((-p2 - sqD) * 0.5 );
    }

    std::vector<double> roots;

    for (auto root : complex_roots)
        if (fabs(root.imag()) < eps)
            roots.push_back(root.real());

    sort(roots.begin(), roots.end());

    return roots;
}
