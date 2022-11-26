// Fill out your copyright notice in the Description page of Project Settings.

#include "TrackingActor.h"

#include <string>
#include <functional>

using namespace std;

// Sets default values
ATrackingActor::ATrackingActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	mesh = CreateDefaultSubobject<UStaticMeshComponent>(FName("Static mesh"));
	SetRootComponent(mesh);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("/Game/StarterContent/Shapes/Shape_Plane.Shape_Plane"));

	if (PlaneMesh.Object)
		mesh->SetStaticMesh(PlaneMesh.Object);
}

// Called when the game starts or when spawned
void ATrackingActor::BeginPlay()
{
	at_td = apriltag_detector_create();
	at_td->quad_decimate = 1.0; // decimate factor
	at_td->quad_sigma = 0.0;	// apply this much low-pass blur to input
	at_td->nthreads = 1;		// use this many cpu threads
	at_td->debug = false;		// print debug output
	at_td->refine_edges = true; // refine tag edges

	function<apriltag_family_t*()> family_functions[] = {tag16h5_create, tag25h9_create, tag36h11_create, tagCircle21h7_create, tagCircle49h12_create, tagCustom48h12_create, tagStandard41h12_create, tagStandard52h13_create};

	apriltag_detector_add_family(at_td, tag36h11_create());


	cv_cap.open(0, CAP_ANY);

	if (!cv_cap.isOpened())
	{
		UE_LOG(LogTemp, Warning, TEXT("Could not open camera"));
	}
	else
	{
		cv_size = Size(cv_cap.get(CAP_PROP_FRAME_WIDTH), cv_cap.get(CAP_PROP_FRAME_HEIGHT));

		camera_texture_2d = UTexture2D::CreateTransient(cv_size.width, cv_size.height, PF_B8G8R8A8);
		camera_texture_2d->MipGenSettings = TMGS_NoMipmaps;

		UMaterialInstanceDynamic *material = UMaterialInstanceDynamic::Create(mesh->GetMaterial(0), this);
		material->SetTextureParameterValue(FName(TEXT("Texture")), camera_texture_2d);
		mesh->SetMaterial(0, material);
	}

	Super::BeginPlay();
}

// Called every frame
void ATrackingActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (cv_cap.isOpened())
	{
		cv_cap >> cv_frame;
		cvtColor(cv_frame, cv_frame_gray, COLOR_BGR2GRAY);

		// Make an image_u8_t header for the Mat data
		image_u8_t im = {.width = cv_frame_gray.cols,
						 .height = cv_frame_gray.rows,
						 .stride = cv_frame_gray.cols,
						 .buf = cv_frame_gray.data};

		zarray_t *detections = apriltag_detector_detect(at_td, &im);

		// Draw detection outlines
		for (int i = 0; i < zarray_size(detections); i++)
		{
			apriltag_detection_t *det;
			zarray_get(detections, i, &det);

			// apriltag_detection_info_t info;
			// info.det = det;
			// info.tagsize = _tagsize;
			// info.fx = 522.12285297286087;
			// info.fy = 531.89050725948039;
			// info.cx = 320;
			// info.cy = 240;

			// apriltag_pose_t pose;
			// estimate_tag_pose(&info, &pose);

			// Matrix4 transformationTag = (det->id >= 0 && det->id < _tagTransformations.size()) ? _tagTransformations[det->id] : Matrix4::translation({});

			//_cameraTransformation = getMatrix(pose).inverted();

			line(cv_frame, Point(det->p[0][0], det->p[0][1]),
				 Point(det->p[1][0], det->p[1][1]),
				 Scalar(0, 0, 0xff), 2);
			line(cv_frame, Point(det->p[0][0], det->p[0][1]),
				 Point(det->p[3][0], det->p[3][1]),
				 Scalar(0, 0xff, 0), 2);
			line(cv_frame, Point(det->p[1][0], det->p[1][1]),
				 Point(det->p[2][0], det->p[2][1]),
				 Scalar(0xff, 0, 0), 2);
			line(cv_frame, Point(det->p[2][0], det->p[2][1]),
				 Point(det->p[3][0], det->p[3][1]),
				 Scalar(0xff, 0, 0), 2);

			string text = to_string(det->id);
			int fontface = FONT_HERSHEY_SCRIPT_SIMPLEX;
			double fontscale = 1.0;
			int baseline;
			Size textsize = getTextSize(text, fontface, fontscale, 2,
										&baseline);
			putText(cv_frame, text, Point(det->c[0] - textsize.width / 2, det->c[1] + textsize.height / 2),
					fontface, fontscale, Scalar(0xff, 0x99, 0), 2);
		}

		cvtColor(cv_frame, cv_frame, COLOR_BGR2BGRA);

		void *texture_data = camera_texture_2d->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
		memcpy(texture_data, cv_frame.data, cv_frame.elemSize() * cv_frame.size().area());
		camera_texture_2d->PlatformData->Mips[0].BulkData.Unlock();
		camera_texture_2d->UpdateResource();
	}
}

void ATrackingActor::BeginDestroy()
{
	Super::BeginDestroy();

	cv_cap.release();

	// destroy tag families
	if (at_td)
	{
		zarray_t *families;
		families = at_td->tag_families;
		for (int i = 0; i < zarray_size(families); i++)
		{
			apriltag_family_t *tf;
			zarray_get(families, i, &tf);

			if (!strcmp(tf->name, "tag36h11"))
			{
				tag36h11_destroy(tf);
			}
			else if (!strcmp(tf->name, "tag25h9"))
			{
				tag25h9_destroy(tf);
			}
			else if (!strcmp(tf->name, "tag16h5"))
			{
				tag16h5_destroy(tf);
			}
			else if (!strcmp(tf->name, "tagCircle21h7"))
			{
				tagCircle21h7_destroy(tf);
			}
			else if (!strcmp(tf->name, "tagCircle49h12"))
			{
				tagCircle49h12_destroy(tf);
			}
			else if (!strcmp(tf->name, "tagStandard41h12"))
			{
				tagStandard41h12_destroy(tf);
			}
			else if (!strcmp(tf->name, "tagStandard52h13"))
			{
				tagStandard52h13_destroy(tf);
			}
			else if (!strcmp(tf->name, "tagCustom48h12"))
			{
				tagCustom48h12_destroy(tf);
			}
		}
		apriltag_detector_destroy(at_td);
	}
}
