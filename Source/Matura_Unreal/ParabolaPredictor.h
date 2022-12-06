// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MatrixTypes.h"

/**
 * 
 */
class MATURA_UNREAL_API ParabolaPredictor {
public:
    struct Parabola {
        double a = 0, b = 0, c = 0;

        Parabola(double a, double b, double c) : a(a), b(b), c(c) {}

        Parabola() {}
        
        static Parabola from2Points(FVector2d p1, FVector2d p2, double a) {
            UE::Geometry::FMatrix2d A;
            A.Row0 = {p1.X, 1};
            A.Row1 = {p2.X, 1};
            
            FVector2d C = {p1.Y - a * p1.X * p1.X,
                         p2.Y - a * p2.X * p2.X};

            FVector2d B = A.Inverse() * C;

            return Parabola(a, B.X, B.Y);
        }

        static Parabola from3Points(FVector2d p1, FVector2d p2, FVector2d p3) {
            UE::Geometry::FMatrix3d A;
            A.Row0 = {p1.X * p1.X, p1.X, 1};
            A.Row1 = {p2.X * p2.X, p2.X, 1};
            A.Row2 = {p3.X * p3.X, p3.X, 1};

            FVector C = {p1.Y, p2.Y, p3.Y};

            FVector B = A.Inverse() * C;//.TransformVector(C);

            return Parabola{B.X, B.Y, B.Z};
        }

        double derivative(double t) const {
            return 2 * a * t + b;
        }

        double derivative2() const {
            return 2 * a;
        }

        double operator()(double t) const {
            return t * t * a / 2 + t * b + c;
        }
    };

    struct Prediction {
        Parabola parabola;
        FVector2d start, speed;

        FVector operator()(double t) const {
            return {start.X + speed.X * t,  start.Y + speed.Y * t, parabola(t)};
        }
    };

private:
    const double _g = -9.807;

    TMap<double, FVector> _points;

    bool generate();

public:
    Prediction _prediction;

    ParabolaPredictor();
    ~ParabolaPredictor();
    
    FVector operator()(double t) const {
        return _prediction(t);
    }

    bool addPoint(double t, FVector p) {
        if (_points.Contains(t))
        {
            UE_LOG(LogTemp, Warning, TEXT("Point already exists in Parabola!"));
            return false;
        }
        _points.Emplace(t, p);

        return generate();
    }

    void clear() {
        _points.Reset();
        _prediction = {};
    }

};

