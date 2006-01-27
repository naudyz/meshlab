/****************************************************************************
* MeshLab                                                           o o     *
* An extendible mesh processor                                    o     o   *
*                                                                _   O  _   *
* Copyright(C) 2005, 2006                                          \/)\/    *
* Visual Computing Lab                                            /\/|      *
* ISTI - Italian National Research Council                           |      *
*                                                                    \      *
* All rights reserved.                                                      *
*                                                                           *
* This program is free software; you can redistribute it and/or modify      *
* it under the terms of the GNU General Public License as published by      *
* the Free Software Foundation; either version 2 of the License, or         *
* (at your option) any later version.                                       *
*                                                                           *
* This program is distributed in the hope that it will be useful,           *
* but WITHOUT ANY WARRANTY; without even the implied warranty of            *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
* GNU General Public License (http://www.gnu.org/licenses/gpl.txt)          *
* for more details.                                                         *
*                                                                           *
****************************************************************************/
/****************************************************************************
  History

 $Log$
 Revision 1.64  2006/01/27 01:13:27  buzzelli
 Added a better distinction beetween critical and non critical error messages

 Revision 1.63  2006/01/26 22:51:57  fmazzant
 removed last links to the Exporter mask

 Revision 1.62  2006/01/26 18:39:20  fmazzant
 moved mask dialog exporter from mashio to meshlab

 Revision 1.61  2006/01/23 15:54:04  fmazzant
 added m.mask = mask to show Mesh information

 Revision 1.60  2006/01/23 01:26:30  buzzelli
 added handling of non critical errors which may occurr during obj file importing

 Revision 1.59  2006/01/22 00:31:14  buzzelli
 adding first rough texture loading support into 3ds file importing

 Revision 1.58  2006/01/20 18:17:10  vannini
 added Restore Color

 Revision 1.57  2006/01/19 17:07:51  fmazzant
 changed struct Format to class Format(QString, QString).
 updated importFormats() and exportFormats() to the new class.

 Revision 1.56  2006/01/19 16:23:21  fmazzant
 changed return 0 in return MeshModel::IOM_ALL on GetExportMaskCapability(...)

 Revision 1.55  2006/01/19 15:59:00  fmazzant
 moved savemaskexporter to mainwindows

 Revision 1.53  2006/01/19 09:36:28  fmazzant
 cleaned up history log

*****************************************************************************/
#include <Qt>
#include <QtGui>

#include "meshio.h"

// temporaneamente prendo la versione corrente dalla cartella test
#include "../../test/io/import_obj.h"
#include "../../test/io/export_obj.h"

#include "../../test/io/import_3ds.h"
#include "../../test/io/export_3ds.h"

#include <wrap/io_trimesh/import_ply.h>
#include <wrap/io_trimesh/export_ply.h>

#include <wrap/io_trimesh/import_stl.h>
#include <wrap/io_trimesh/import_off.h>

#include<vcg/complex/trimesh/update/bounding.h>
#include <wrap/io_trimesh/export.h>
#include <wrap/io_trimesh/io_mask.h>
#include <wrap/ply/plylib.h>
#include <vcg/complex/trimesh/update/normal.h>

#include <QMessageBox>
#include <QFileDialog>

using namespace vcg;


bool ExtraMeshIOPlugin::open(const QString &formatName, QString &fileName,MeshModel &m, int& mask,CallBackPos *cb,QWidget *parent)
{
	if (fileName.isEmpty())
		fileName = QFileDialog::getOpenFileName(parent,tr("Open File"),"../sample","Obj files (*.obj)");
	
  mask=0; // just to be sure...
	
	if (!fileName.isEmpty())
	{
		// initializing progress bar status
		if (cb != NULL)
			(*cb)(0, "Loading...");

		// this change of dir is needed for subsequent texture/material loading
		QString FileNameDir = fileName.left(fileName.lastIndexOf("/")); 
		QDir::setCurrent(FileNameDir);

		QString errorMsgFormat = "Error encountered while loading file:\n\"%1\"\n\nError details: %2";
		string filename = fileName.toUtf8().data();

		bool bUpdatedNormals = false;

		if(formatName.toUpper() == tr("OBJ"))
		{
			vcg::tri::io::ObjInfo oi;	
			oi.cb = cb;
			vcg::tri::io::ImporterOBJ<CMeshO>::LoadMask(filename.c_str(), mask, oi);

			if(mask & vcg::tri::io::Mask::IOM_WEDGTEXCOORD) 
			{
				qDebug("Has Wedge Text Coords\n");
				m.cm.face.EnableWedgeTex();
			}

			int result = vcg::tri::io::ImporterOBJ<CMeshO>::Open(m.cm, filename.c_str(), oi);
			if (result != vcg::tri::io::ImporterOBJ<CMeshO>::E_NOERROR)
			{
				if (result & vcg::tri::io::ImporterOBJ<CMeshO>::E_NON_CRITICAL_ERROR)
					QMessageBox::warning(parent, tr("OBJ Opening Warning"), vcg::tri::io::ImporterOBJ<CMeshO>::ErrorMsg(result));
				else
				{
					QMessageBox::critical(parent, tr("OBJ Opening Error"), errorMsgFormat.arg(fileName, vcg::tri::io::ImporterOBJ<CMeshO>::ErrorMsg(result)));
					return false;
				}
			}

			if(mask & MeshModel::IOM_WEDGNORMAL)
				bUpdatedNormals = true;
		}
		else if (formatName.toUpper() == tr("PLY"))
		{
			vcg::tri::io::ImporterPLY<CMeshO>::LoadMask(filename.c_str(), mask); 
		  
			if(mask&MeshModel::IOM_VERTQUALITY) qDebug("Has Vertex Quality\n");
			if(mask&MeshModel::IOM_FACEQUALITY) qDebug("Has Face Quality\n");
			if(mask&MeshModel::IOM_FACECOLOR)		qDebug("Has Face Color\n");
			if(mask&MeshModel::IOM_VERTCOLOR)		qDebug("Has Vertex Color\n");
			if(mask&MeshModel::IOM_WEDGTEXCOORD) 
			{
				qDebug("Has Wedge Text Coords\n");
				m.cm.face.EnableWedgeTex();
			}
			
			int result = vcg::tri::io::ImporterPLY<CMeshO>::Open(m.cm, filename.c_str(), cb);
			if (result != ::vcg::ply::E_NOERROR)
			{
				QMessageBox::warning(parent, tr("PLY Opening Error"), errorMsgFormat.arg(fileName, vcg::tri::io::ImporterPLY<CMeshO>::ErrorMsg(result)));
				return false;
			}
		}
		else if (formatName.toUpper() == tr("OFF"))
		{
			int result = vcg::tri::io::ImporterOFF<CMeshO>::Open(m.cm, filename.c_str());
			if (result != 0)  // OFFCodes enum is protected
			{
				QMessageBox::warning(parent, tr("OFF Opening Error"), errorMsgFormat.arg(fileName, vcg::tri::io::ImporterOFF<CMeshO>::ErrorMsg(result)));
				return false;
			}

			CMeshO::FaceIterator fi = m.cm.face.begin();
			for (; fi != m.cm.face.end(); ++fi)
				face::ComputeNormalizedNormal(*fi);
		}
		else if (formatName.toUpper() == tr("STL"))
		{
			int result = vcg::tri::io::ImporterSTL<CMeshO>::Open(m.cm, filename.c_str(), cb);
			if (result != vcg::tri::io::ImporterSTL<CMeshO>::E_NOERROR)
			{
				QMessageBox::warning(parent, tr("STL Opening Error"), errorMsgFormat.arg(fileName, vcg::tri::io::ImporterSTL<CMeshO>::ErrorMsg(result)));
				return false;
			}

			CMeshO::FaceIterator fi = m.cm.face.begin();
			for (; fi != m.cm.face.end(); ++fi)
				face::ComputeNormalizedNormal(*fi);
		}
		else if (formatName.toUpper() == tr("3DS"))
		{
			vcg::tri::io::_3dsInfo info;	
			info.cb = cb;
			// TODO: decomment this code when implementation of loadmask will be done
			/*vcg::tri::io::Importer3DS<CMeshO>::LoadMask(filename.c_str(), mask, info);

			if(mask & MeshModel::IOM_WEDGTEXCOORD) 
			{
				qDebug("Has Wedge Text Coords\n");
				*//*TODO: abilitare solo quando necessario*//*m.cm.face.EnableWedgeTex();*//*
			}*/

			int result = vcg::tri::io::Importer3DS<CMeshO>::Open(m.cm, filename.c_str(), info);
			if (result != vcg::tri::io::Importer3DS<CMeshO>::E_NOERROR)
			{
				QMessageBox::warning(parent, tr("3DS Opening Error"), errorMsgFormat.arg(fileName, vcg::tri::io::Importer3DS<CMeshO>::ErrorMsg(result)));
				return false;
			}

			if(mask & MeshModel::IOM_WEDGNORMAL)
				bUpdatedNormals = true;
		}

		vcg::tri::UpdateBounding<CMeshO>::Box(m.cm);					// updates bounding box
		if (!bUpdatedNormals) 
			vcg::tri::UpdateNormals<CMeshO>::PerVertex(m.cm);		// updates normals

		if (cb != NULL)
			(*cb)(100, "Done");

		m.storeVertexColor();
		
		m.mask = mask;

		return true;
	}

	return false;
}

bool ExtraMeshIOPlugin::save(const QString &formatName,QString &fileName, MeshModel &m, int &mask, vcg::CallBackPos *cb, QWidget *parent)
{
	QString errorMsgFormat = "Error encountered while exportering file %1:\n%2";
	string filename = fileName.toUtf8().data();
	string ex = formatName.toUtf8().data();

	if(formatName.toUpper() == tr("OBJ"))
	{	
		int result = vcg::tri::io::ExporterOBJ<CMeshO>::Save(m.cm,filename.c_str(),false,mask,cb);//salva esclusivamente in formato ASCII
		if(result != vcg::tri::io::ExporterOBJ<CMeshO>::E_NOERROR )
		{
			QMessageBox::warning(parent, tr("OBJ Saving Error"), errorMsgFormat.arg(fileName, vcg::tri::io::ExporterOBJ<CMeshO>::ErrorMsg(result)));
			return false;
		}
		return true;
	}

	if(formatName.toUpper() == tr("PLY"))
	{		
		int result = vcg::tri::io::Exporter<CMeshO>::Save(m.cm,filename.c_str(),mask,cb);
		if(result != 0)
		{
			QMessageBox::warning(parent, tr("PLY Saving Error"), errorMsgFormat.arg(fileName, vcg::tri::io::Exporter<CMeshO>::ErrorMsg(result)));
			return false;
		}
		return true;
	}

	if(formatName.toUpper() == tr("OFF"))
	{
		int result = vcg::tri::io::Exporter<CMeshO>::Save(m.cm,filename.c_str(),cb);
		if(result != 0)
		{
			//QMessageBox::warning(parent, tr("OFF Saving Error"), errorMsgFormat.arg(fileName, vcg::tri::io::ExporterOFF<CMeshO>::ErrorMsg(result)));
			return false;
		}
		return true;
	}

	if(formatName.toUpper() == tr("STL"))
	{
		int result = vcg::tri::io::Exporter<CMeshO>::Save(m.cm,filename.c_str(),mask,cb);
		if(result != 0)
		{
			QMessageBox::warning(parent, tr("Mesh Saving Error"), errorMsgFormat.arg(fileName, vcg::tri::io::Exporter<CMeshO>::ErrorMsg(result)));
			return false;
		}
		return true;

	}

	if(formatName.toUpper() == tr("3DS"))
	{	
		int result = vcg::tri::io::Exporter3DS<CMeshO>::Save(m.cm,filename.c_str(),true,mask,cb);//salva esclusivamente in formato binario
		if(result!=0)
		{
			QMessageBox::warning(parent, tr("3DS Saving Error"), errorMsgFormat.arg(fileName, vcg::tri::io::Exporter3DS<CMeshO>::ErrorMsg(result)));
			return false;
		}
		return true;
	}

	QMessageBox::warning(parent, "Unknow type", "file's extension not supported!!!");

	return false;
}

QList<MeshIOInterface::Format> ExtraMeshIOPlugin::importFormats() const
{
	QList<Format> formatList;
	formatList << Format("Stanford Polygon File Format"	,tr("PLY"));
	formatList << Format("Alias Wavefront Object"		,tr("OBJ"));
	formatList << Format("Object File Format"			,tr("OFF"));
	formatList << Format("STL File Format"				,tr("STL"));
	formatList << Format("3D-Studio File Format"		,tr("3DS"));
	return formatList;
}

QList<MeshIOInterface::Format> ExtraMeshIOPlugin::exportFormats() const
{
	QList<Format> formatList;
	formatList << Format("Stanford Polygon File Format"	,tr("PLY"));
	formatList << Format("Alias Wavefront Object"		,tr("OBJ"));
	formatList << Format("Object File Format"			,tr("OFF"));
	formatList << Format("STL File Format"				,tr("STL"));
	formatList << Format("3D-Studio File Format"		,tr("3DS"));
	return formatList;
}

int ExtraMeshIOPlugin::GetExportMaskCapability(QString &format) const
{
	if(format.toUpper() == tr("OBJ")){return vcg::tri::io::ExporterOBJ<CMeshO>::GetExportMaskCapability();}
	if(format.toUpper() == tr("PLY")){return MeshModel::IOM_ALL;}
	if(format.toUpper() == tr("OFF")){return MeshModel::IOM_ALL;}
	if(format.toUpper() == tr("STL")){return MeshModel::IOM_ALL;}
	if(format.toUpper() == tr("3DS")){return vcg::tri::io::Exporter3DS<CMeshO>::GetExportMaskCapability();}
}

Q_EXPORT_PLUGIN(ExtraMeshIOPlugin)