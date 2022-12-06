// Fill out your copyright notice in the Description page of Project Settings.


#include "ParabolaPredictor.h"
#include <vector>

using namespace std;

ParabolaPredictor::ParabolaPredictor()
{
}

ParabolaPredictor::~ParabolaPredictor()
{
}

bool ParabolaPredictor::generate()
{
        vector<FVector> points;
        vector<double> times;

        for (auto [t, p]: _points) {
            times.push_back({t});
            points.push_back({p});
        }

        if (_points.Num() < 3)
            return false;

        double sumOffLine = 0;
        double sumOffPar = 0;

        for (int i = 0; i < int(points.size() - 2); i++) {
            auto par = Parabola::from3Points(
                    {times[i], points[i].Z},
                    {times[i + 1], points[i + 1].Z},
                    {times[i + 2], points[i + 2].Z});

            double offPar = abs(par.derivative2() - _g);

            if (offPar >= 0.1f)
                return false; // second derivative of heights is too far off, may also be measurement error though
            sumOffPar += offPar;

            FVector2d a = FVector2d{points[i + 1].X, points[i + 1].Y} - FVector2d{points[i].X, points[i].Y};
            FVector2d b = FVector2d{points[i + 2].X, points[i + 2].Y} - FVector2d{points[i + 1].X, points[i + 1].Y};

            double offLine = abs(a.Dot(b) / a.Length() / b.Length());

            if (offLine < cos(10))
                return false; // points aren't really on a line, may also be measurement error though

            sumOffLine += offLine;
        }

        if (sumOffPar / double(points.size()-2) > 0.05f)
            return false; // second derivative of heights is too far off, probably isn't movement influenced by gravity

        if (sumOffLine / double(points.size()-2) < cos(5))
            return false; // points aren't really on a line, probably isn't flying through the air


        Prediction newPrediction;
        newPrediction.speed = FVector2d{points.back().X - points[0].X, points.back().Y - points[0].Y} /
                (times.back() - times[0]);

        newPrediction.start = FVector2d{points[0].X, points[0].Y} + newPrediction.speed * (-times[0]);

        double averageB = 0;
        double averageC = 0;

        for (int i = 0; i < int(points.size() - 1); i++) {
            auto par = Parabola::from2Points({times[i], points[i].Z}, {times[i + 1], points[i + 1].Z}, _g / 2);
            averageB += par.b;
            averageC += par.c;
        }

        newPrediction.parabola = Parabola(_g / 2, averageB / double(points.size()-1), averageC / double(points.size()-1));
    
        _prediction = newPrediction;
        return true;
    }
