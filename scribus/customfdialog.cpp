/***************************************************************************
                          customfdialog.cpp  -  description
                             -------------------
    begin                : Fri Nov 30 2001
    copyright            : (C) 2001 by Franz Schmid
    email                : Franz.Schmid@altmuehlnet.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "customfdialog.h"
#include "customfdialog.moc"
#include <qimage.h>
#include <qpixmap.h>
#include <qpainter.h>
#include <qfileinfo.h>
#include <qdir.h>
#include <qdom.h>
#include <qtextcodec.h>
extern QImage LoadPict(QString fn);
extern bool loadText(QString nam, QString *Buffer);
extern QPixmap loadIcon(QString nam);
extern QString DocDir;

ImIconProvider::ImIconProvider(QWidget *pa) : QFileIconProvider(pa)
{
  fmts.clear();
  fmts += "eps";
  fmts += "gif";
  fmts += "png";
  fmts += "jpg";
  fmts += "xpm";
  fmts += "tif";
  fmts += "bmp";
  fmts += "pbm";
  fmts += "pgm";
  fmts += "ppm";
  fmts += "xbm";
  fmts += "xpm";
  imagepm = loadIcon("image.png");
  pspm = loadIcon("postscript.png");
  txtpm = loadIcon("txt.png");
  docpm = loadIcon("doc.png");
  pdfpm = loadIcon("pdf.png");
}

const QPixmap * ImIconProvider::pixmap(const QFileInfo &fi)
{
	QString ext = fi.extension(false).lower();
	if (fmts.contains(ext))
   	{
		return &imagepm;
		}
	else
		{
		if (ext == "ps")
			return &pspm;
		if (ext == "txt")
			return &txtpm;
		if (ext == "scd")
			return &docpm;
		if (ext == "sla")
			return &docpm;
		if (ext == "pdf")
			return &pdfpm;
		return QFileIconProvider::pixmap(fi);
		}
}

FDialogPreview::FDialogPreview(QWidget *pa) : QLabel(pa)
{
	setAlignment(AlignLeft | AlignTop);
	setMinimumSize( QSize( 100, 100 ) );
	setScaledContents( false );
	setEraseColor( white );
	setFrameShape( QLabel::WinPanel );
	setFrameShadow( QLabel::Sunken );
}

void FDialogPreview::GenPreview(QString name)
{
	QPixmap pm;
	QString Buffer = "";
	setText("");
	QFileInfo fi = QFileInfo(name);
	if (fi.isDir())
		return;
	QString ex = fi.extension(false).upper();
	QStrList imfo = QImageIO::inputFormats();
	if (ex == "JPG")
		ex = "JPEG";
	if ((imfo.contains(ex))||(ex=="PS")||(ex=="EPS")||(ex=="PDF")||(ex=="TIF"))
		{
		QImage im = LoadPict(name);
		if (!im.isNull())
			{
			int ix = im.width();
			int iy = im.height();
			QString tmp = "";
			if ((im.width() > width()-5) || (im.height() > height()-20))
				{
				QImage im2;
				float sx = im.width() / 
				  static_cast<float>(width()-5);
				float sy = im.height() / 
				  static_cast<float>(height()-20);
				im2 = sy < sx ?  im.smoothScale(qRound(im.width() / sx), qRound(im.height() / sx)) : im.smoothScale(qRound(im.width() / sy), qRound(im.height() / sy));
				im = im2;
				im2.detach();
				}
			QPainter p;
			pm = QPixmap(width(), height());
			pm.fill(white);
			p.begin(&pm);
			p.drawImage(0, 0, im);
			p.drawText(2, height()-5, tr("Size:")+" "+tmp.setNum(ix)+" x "+tmp.setNum(iy));
			p.end();
			setPixmap(pm);
			}
		}
	else
		{
		if (loadText(name, &Buffer))
			{
			if (Buffer.startsWith("<SCRIBUS"))
				{
				QDomDocument docu("scridoc");
				if(!docu.setContent(Buffer))
					return;
				QDomElement elem=docu.documentElement();
				if ((elem.tagName() != "SCRIBUS") && (elem.tagName() != "SCRIBUSUTF8"))
					return;
				QDomNode DOC=elem.firstChild();
				QDomElement dc=DOC.toElement();
				QString Tit = tr("Title:")+" ";
				QString ti2 = dc.attribute("TITLE");
				if (ti2 == "")
					ti2= tr("No Title");
				Tit += ti2+"\n";
				QString Aut = tr("Author:")+" ";
				QString au2 = dc.attribute("AUTHOR");
				if (au2 == "")
					au2 = tr("Unknown");
				Aut += au2;
				setText(tr("Scribus-Document")+"\n\n"+Tit+Aut);
				}
			else
				setText(Buffer.left(200));
			}
		}
}	

void FDialogPreview::previewUrl( const QUrl &url )
{
	if (url.isLocalFile())
		GenPreview(url.path());
}

CustomFDialog::CustomFDialog(QWidget *pa, QString cap, QString filter, bool Pre, bool mod, bool comp, bool cod)
						 : QFileDialog(QString::null, filter, pa, 0, true)
{
 	setIcon(loadIcon("AppIcon.xpm"));
 	setCaption(cap);
	cDir = QDir();
	setDir(cDir);
	setIconProvider(new ImIconProvider(this));
	setContentsPreviewEnabled( true );
	FDialogPreview *pw = new FDialogPreview( this );
	setContentsPreview( pw, pw );
	if (comp)
		{
		Layout = new QFrame(this);
		Layout1 = new QHBoxLayout(Layout);
		Layout1->setSpacing( 6 );
		Layout1->setMargin( 0 );
		SaveZip = new QCheckBox(Layout, "test");
		SaveZip->setText(tr("Compress File"));
		Layout1->addWidget(SaveZip);
		QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
  	Layout1->addItem( spacer );
		}
	if (mod)
		setMode(QFileDialog::ExistingFile);
	else
		{
		setMode(QFileDialog::AnyFile);
		if (comp)
			addWidgets(0, Layout, 0);
		}
	if (cod)
		{
		LayoutC = new QFrame(this);
		Layout1C = new QHBoxLayout(LayoutC);
		Layout1C->setSpacing( 0 );
		Layout1C->setMargin( 4 );
		TxCodeT = new QLabel(this);
		TxCodeT->setText(tr("Encoding:"));
		TxCodeM = new QComboBox(true, LayoutC, "Cod");
		TxCodeM->setEditable(false);
		TxCodeM->insertItem("ISO 8859-1");
		TxCodeM->insertItem("ISO 8859-2");
		TxCodeM->insertItem("ISO 8859-3");
		TxCodeM->insertItem("ISO 8859-4");
		TxCodeM->insertItem("ISO 8859-5");
		TxCodeM->insertItem("ISO 8859-6");
		TxCodeM->insertItem("ISO 8859-7");
		TxCodeM->insertItem("ISO 8859-8");
		TxCodeM->insertItem("ISO 8859-9");
		TxCodeM->insertItem("ISO 8859-10");
		TxCodeM->insertItem("ISO 8859-13");
		TxCodeM->insertItem("ISO 8859-14");
		TxCodeM->insertItem("ISO 8859-15");
		TxCodeM->insertItem("utf8");
		TxCodeM->insertItem("KOI8-R");
		TxCodeM->insertItem("KOI8-U");
		TxCodeM->insertItem("CP1250");
		TxCodeM->insertItem("CP1251");
		TxCodeM->insertItem("CP1252");
		TxCodeM->insertItem("CP1253");
		TxCodeM->insertItem("CP1254");
		TxCodeM->insertItem("CP1255");
		TxCodeM->insertItem("CP1256");
		TxCodeM->insertItem("CP1257");
		QString localEn = QTextCodec::codecForLocale()->name();
		bool hasIt = false;
		for (int cc = 0; cc < TxCodeM->count(); cc++)
			{
			if (TxCodeM->text(cc) == localEn)
				{
				TxCodeM->setCurrentItem(cc);
				hasIt = true;
				break;
				}
			}
		if (!hasIt)
			{
			TxCodeM->insertItem(localEn);
			TxCodeM->setCurrentItem(TxCodeM->count()-1);
			}
		TxCodeM->setMinimumSize(QSize(200, 0));
		Layout1C->addWidget(TxCodeM);
		QSpacerItem* spacer2 = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
  	Layout1C->addItem( spacer2 );
		addWidgets(TxCodeT, LayoutC, 0);
		}
	if (Pre)
		setPreviewMode( QFileDialog::Contents );
	else
		setPreviewMode( QFileDialog::NoPreview );
	setViewMode( QFileDialog::List );
	if (comp)
		connect(SaveZip, SIGNAL(clicked()), this, SLOT(HandleComp()));
	HomeB = new QToolButton(this);
	HomeB->setIconSet(loadIcon("gohome.png"));
	HomeB->setTextLabel(tr("Moves to your Document Directory.\nThis can be set in the Preferences."));
	connect(HomeB, SIGNAL(clicked()), this, SLOT(slotHome()));
	addToolButton(HomeB);
}

CustomFDialog::~CustomFDialog()
{
	setIconProvider(0);
	cDir.setCurrent(dir()->path());
}

void CustomFDialog::slotHome()
{
	setDir(QDir(DocDir));
}

void CustomFDialog::HandleComp()
{
	QString tmp;
	tmp = selectedFile();
	if (SaveZip->isChecked())
		{
		if (tmp.right(3) != ".gz")
			{
			if (tmp.right(3) == "svg")
				tmp = tmp+"z";
			else
				tmp = tmp+".gz";
			}
		setSelection(tmp);
		}
	else
		{
		int en = tmp.findRev(".gz");
		if (en > 0)
			tmp.remove(en,3);
		else
			{
			en = tmp.findRev("z");
			if (en > 0)
				tmp.remove(en,1);
			}
		}
	setSelection(tmp);
}
