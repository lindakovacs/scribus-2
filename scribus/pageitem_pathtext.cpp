/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/
/***************************************************************************
                          pageitem.cpp  -  description
                             -------------------
    begin                : Sat Apr 7 2001
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

#include "pageitem_pathtext.h"
#include <QGridLayout>
#include <QImage>
#include <QLabel>
#include <QList>
#include <QPainterPath>

#include <cassert>

#include "scconfig.h"

#include "commonstrings.h"
#include "pageitem.h"
#include "prefsmanager.h"
#include "scpage.h"
#include "scpainter.h"
#include "scpaths.h"
#include "scraction.h"

#include "scribusstructs.h"
#include "scribusdoc.h"
#include "undomanager.h"
#include "undostate.h"
#include "util.h"
#include "util_math.h"

using namespace std;

PageItem_PathText::PageItem_PathText(ScribusDoc *pa, double x, double y, double w, double h, double w2, QString fill, QString outline)
	: PageItem(pa, PageItem::PathText, x, y, w, h, w2, fill, outline), itemRenderText(pa)
{
	firstChar = 0;
	MaxChars = itemText.length();
}


void PageItem_PathText::layout()
{
	QImage pgPix(10, 10, QImage::Format_ARGB32_Premultiplied);
	QRectF rd; // = QRect(0,0,9,9);
	ScPainter *painter = new ScPainter(&pgPix, pgPix.width(), pgPix.height());	
	DrawObj(painter, rd);
	painter->end();
	delete painter;
	updatePolyClip();
}


void PageItem_PathText::DrawObj_Item(ScPainter *p, QRectF cullingArea)
{
	itemText.invalidateAll();
	firstChar = 0;
	MaxChars = 0;
	int a;
	QString chstr, chstr2, chstr3;
	double dx;
	FPoint point = FPoint(0, 0);
	FPoint tangent = FPoint(0, 0);
	QColor tmp;
	CurX = m_textDistanceMargins.Left;
	QString cachedStroke = "";
	QString cachedFill = "";
	double cachedFillShade = -1;
	double cachedStrokeShade = -1;
	QString actStroke = "";
	QString actFill = "";
	double actFillShade = -1;
	double actStrokeShade = -1;
	QColor cachedFillQ;
	QColor cachedStrokeQ;
	if (!m_Doc->layerOutline(LayerID))
	{
		if (PoShow)
		{
			p->setupPolygon(&PoLine, false);
			if (NamedLStyle.isEmpty())
			{
				if ((!patternStrokeVal.isEmpty()) && (m_Doc->docPatterns.contains(patternStrokeVal)))
				{
					if (patternStrokePath)
					{
						QPainterPath guidePath = PoLine.toQPainterPath(false);
						DrawStrokePattern(p, guidePath);
					}
					else
					{
						p->setPattern(&m_Doc->docPatterns[patternStrokeVal], patternStrokeScaleX, patternStrokeScaleY, patternStrokeOffsetX, patternStrokeOffsetY, patternStrokeRotation, patternStrokeSkewX, patternStrokeSkewY, patternStrokeMirrorX, patternStrokeMirrorY);
						p->setStrokeMode(ScPainter::Pattern);
						p->strokePath();
					}
				}
				else if (GrTypeStroke > 0)
				{
					if ((!gradientStrokeVal.isEmpty()) && (!m_Doc->docGradients.contains(gradientStrokeVal)))
						gradientStrokeVal = "";
					if (!(gradientStrokeVal.isEmpty()) && (m_Doc->docGradients.contains(gradientStrokeVal)))
						stroke_gradient = m_Doc->docGradients[gradientStrokeVal];
					if (stroke_gradient.Stops() < 2) // fall back to solid stroking if there are not enough colorstops in the gradient.
					{
						if (lineColor() != CommonStrings::None)
						{
							p->setBrush(strokeQColor);
							p->setStrokeMode(ScPainter::Solid);
						}
						else
							p->setStrokeMode(ScPainter::None);
					}
					else
					{
						p->setStrokeMode(ScPainter::Gradient);
						p->stroke_gradient = stroke_gradient;
						if (GrTypeStroke == 6)
							p->setGradient(VGradient::linear, FPoint(GrStrokeStartX, GrStrokeStartY), FPoint(GrStrokeEndX, GrStrokeEndY), FPoint(GrStrokeStartX, GrStrokeStartY), GrStrokeScale, GrStrokeSkew);
						else
							p->setGradient(VGradient::radial, FPoint(GrStrokeStartX, GrStrokeStartY), FPoint(GrStrokeEndX, GrStrokeEndY), FPoint(GrStrokeFocalX, GrStrokeFocalY), GrStrokeScale, GrStrokeSkew);
					}
					p->strokePath();
				}
				else if (lineColor() != CommonStrings::None)
				{
					p->setStrokeMode(ScPainter::Solid);
					p->strokePath();
				}
			}
			else
			{
				p->setStrokeMode(ScPainter::Solid);
				multiLine ml = m_Doc->MLineStyles[NamedLStyle];
				QColor tmp;
				for (int it = ml.size()-1; it > -1; it--)
				{
					if (ml[it].Color != CommonStrings::None) // && (ml[it].Width != 0))
					{
						SetQColor(&tmp, ml[it].Color, ml[it].Shade);
						p->setPen(tmp, ml[it].Width, static_cast<Qt::PenStyle>(ml[it].Dash), static_cast<Qt::PenCapStyle>(ml[it].LineEnd), static_cast<Qt::PenJoinStyle>(ml[it].LineJoin));
						p->strokePath();
					}
				}
			}
		}
	}
	double totalTextLen = 0.0;
	double totalCurveLen = 0.0;
	double extraOffset = 0.0;
	if (itemText.length() != 0)
	{
		CurX += itemText.charStyle(0).fontSize() * itemText.charStyle(0).tracking() / 10000.0;
		totalTextLen += itemText.charStyle(0).fontSize() * itemText.charStyle(0).tracking() / 10000.0;
	}
	itemRenderText.clear();
	itemRenderText.setDoc(m_Doc);
	itemRenderText.setDefaultStyle(itemText.defaultStyle());
	for (a = firstChar; a < itemText.length(); ++a)
	{
		CharStyle nstyle = itemText.charStyle(a);
		ParagraphStyle pstyle = itemText.paragraphStyle(a);
        chstr = itemText.text(a, 1);
        if ((chstr[0] == SpecialChars::OBJECT) && (itemText.hasObject(a)))
		{
			int pot = itemRenderText.length();
			itemRenderText.insertObject(pot, itemText.object(a)->inlineCharID);
		}
		else
		{
			if (!(chstr[0] == SpecialChars::PARSEP || chstr[0] == SpecialChars::TAB || chstr[0] == SpecialChars::LINEBREAK))
				chstr = ExpandToken(a);
			for (int cc = 0; cc < chstr.count(); cc++)
			{
				int pot = itemRenderText.length();
				itemRenderText.insertChars(pot, chstr.mid(cc, 1));
				itemRenderText.applyStyle(pot, pstyle);
				itemRenderText.applyCharStyle(pot, 1, nstyle);
			}
		}
		chstr.clear();
	}
	textLayout.setStory(&itemRenderText);
	int spaceCount = 0;
	double wordExtra = 0;
	for (a = firstChar; a < itemRenderText.length(); ++a)
	{
        GlyphLayout* glyphs = itemRenderText.getGlyphs(a);
        chstr = itemRenderText.text(a, 1);
		if (chstr[0] == SpecialChars::PAGENUMBER || chstr[0] == SpecialChars::PARSEP || chstr[0] == SpecialChars::PAGECOUNT
			|| chstr[0] == SpecialChars::TAB || chstr[0] == SpecialChars::LINEBREAK)
			continue;
		if (chstr[0] == SpecialChars::BLANK)
			spaceCount++;
		if (a < itemRenderText.length()-1)
			chstr += itemRenderText.text(a+1, 1);
        glyphs->yadvance = 0;
        layoutGlyphs(itemRenderText.charStyle(a), chstr, itemRenderText.flags(a), *glyphs);
        glyphs->shrink();
        if (itemRenderText.hasObject(a))
            totalTextLen += (itemRenderText.object(a)->width() + itemRenderText.object(a)->lineWidth()) * glyphs->scaleH;
		else
            totalTextLen += glyphs->wide()+itemRenderText.charStyle(a).fontSize() * itemRenderText.charStyle(a).tracking() / 10000.0;
	}
	for (int segs = 0; segs < PoLine.size()-3; segs += 4)
	{
		totalCurveLen += PoLine.lenPathSeg(segs);
	}
	if ((itemRenderText.paragraphStyle(0).alignment() != 0) && (totalCurveLen >= totalTextLen + m_textDistanceMargins.Left))
	{
		if (itemRenderText.paragraphStyle(0).alignment() == 2)
		{
			CurX = totalCurveLen  - totalTextLen;
			CurX -= m_textDistanceMargins.Left;
		}
		if (itemRenderText.paragraphStyle(0).alignment() == 1)
			CurX = (totalCurveLen - totalTextLen) / 2.0;
		if (itemRenderText.paragraphStyle(0).alignment() == 3)
		{
			if (spaceCount != 0)
			{
				extraOffset = 0;
				wordExtra = (totalCurveLen - m_textDistanceMargins.Left  - totalTextLen) / static_cast<double>(spaceCount);
			}
			else
			{
				extraOffset = (totalCurveLen - m_textDistanceMargins.Left  - totalTextLen) / static_cast<double>(itemRenderText.length());
				wordExtra = 0;
			}
		}
		if (itemRenderText.paragraphStyle(0).alignment() == 4)
			extraOffset = (totalCurveLen - m_textDistanceMargins.Left - totalTextLen) / static_cast<double>(itemRenderText.length());
	}
	QPainterPath guidePath = PoLine.toQPainterPath(false);
	QList<QPainterPath> pathList = decomposePath(guidePath);
	QPainterPath currPath = pathList[0];
	int currPathIndex = 0;
	for (a = firstChar; a < itemRenderText.length(); ++a)
	{
		CurY = 0;
		GlyphLayout* glyphs = itemRenderText.getGlyphs(a);
		PathData* pdata = & (textLayout.point(a));
        chstr = itemRenderText.text(a,1);
		if (chstr[0] == SpecialChars::PAGENUMBER || chstr[0] == SpecialChars::PARSEP || chstr[0] == SpecialChars::PAGECOUNT
			|| chstr[0] == SpecialChars::TAB || chstr[0] == SpecialChars::LINEBREAK)
			continue;
		if (a < itemRenderText.length()-1)
			chstr += itemRenderText.text(a+1, 1);
		glyphs->yadvance = 0;
		layoutGlyphs(itemRenderText.charStyle(a), chstr, itemRenderText.flags(a), *glyphs);
		glyphs->shrink();                                                           // HACK
        if (itemRenderText.hasObject(a))
			dx = (itemRenderText.object(a)->width() + itemRenderText.object(a)->lineWidth()) * glyphs->scaleH / 2.0;
		else
			dx = glyphs->wide() / 2.0;
		CurX += dx;

		double currPerc = currPath.percentAtLength(CurX);
		if (currPerc >= 0.9999999)
		{
			currPathIndex++;
			if (currPathIndex == pathList.count())
			{
				MaxChars = a;
				break;
			}
			currPath = pathList[currPathIndex];
			CurX = dx;
			currPerc = currPath.percentAtLength(CurX);
		}
		double currAngle = currPath.angleAtPercent(currPerc);
		if (currAngle <= 180.0)
			currAngle *= -1.0;
		else
			currAngle = 360.0 - currAngle;
		QPointF currPoint = currPath.pointAtPercent(currPerc);
		tangent = FPoint(cos(currAngle * M_PI / 180.0), sin(currAngle * M_PI / 180.0));
		point = FPoint(currPoint.x(), currPoint.y());
		glyphs->xoffset = 0;
		pdata->PtransX = point.x();
		pdata->PtransY = point.y();
		pdata->PRot    = currAngle * M_PI / 180.0;
		pdata->PDx     = dx;
		QTransform trafo = QTransform( 1, 0, 0, -1, -dx, 0 );
		if (textPathFlipped)
			trafo *= QTransform(1, 0, 0, -1, 0, 0);
		if (textPathType == 0)
			trafo *= QTransform( tangent.x(), tangent.y(), tangent.y(), -tangent.x(), point.x(), point.y() ); // ID's Rainbow mode
		else if (textPathType == 1)
			trafo *= QTransform( 1, 0, 0, -1, point.x(), point.y() ); // ID's Stair Step mode
		else if (textPathType == 2)
		{
			double a = 1;
			if (tangent.x() < 0)
				a = -1;
			if (fabs(tangent.x()) > 0.1)
				trafo *= QTransform( a, (tangent.y() / tangent.x()) * a, 0, -1, point.x(), point.y() ); // ID's Skew mode
			else
				trafo *= QTransform( a, 4 * a, 0, -1, point.x(), point.y() );
		}
		QTransform sca = p->worldMatrix();
		trafo *= sca;
		p->save();
		QTransform savWM = p->worldMatrix();
		p->setWorldMatrix(trafo);
		if (!m_Doc->RePos)
		{
			actFill = itemRenderText.charStyle(a).fillColor();
			actFillShade = itemRenderText.charStyle(a).fillShade();
			if (actFill != CommonStrings::None)
			{
				p->setFillMode(ScPainter::Solid);
				if ((cachedFillShade != actFillShade) || (cachedFill != actFill))
				{
					SetQColor(&tmp, actFill, actFillShade);
					p->setBrush(tmp);
					cachedFillQ = tmp;
					cachedFill = actFill;
					cachedFillShade = actFillShade;
				}
				else
					p->setBrush(cachedFillQ);
			}
			else
				p->setFillMode(ScPainter::None);
			actStroke = itemRenderText.charStyle(a).strokeColor();
			actStrokeShade = itemRenderText.charStyle(a).strokeShade();
			if (actStroke != CommonStrings::None)
			{
				if ((cachedStrokeShade != actStrokeShade) || (cachedStroke != actStroke))
				{
					SetQColor(&tmp, actStroke, actStrokeShade);
					p->setPen(tmp, 1, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin);
					cachedStrokeQ = tmp;
					cachedStroke = actStroke;
					cachedStrokeShade = actStrokeShade;
				}
				else
					p->setPen(cachedStrokeQ, 1, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin);
			}
			p->translate(0.0, BaseOffs);
            if (itemRenderText.hasObject(a))
                DrawObj_Embedded(p, cullingArea, itemRenderText.charStyle(a), itemRenderText.object(a));
			else
				drawGlyphs(p, itemRenderText.charStyle(a), itemRenderText.flags(a), *glyphs);
		}
		p->setWorldMatrix(savWM);
		p->restore();
		MaxChars = a+1;
		CurX -= dx;
        if (itemRenderText.hasObject(a))
			CurX += (itemRenderText.object(a)->width() + itemRenderText.object(a)->lineWidth()) * glyphs->scaleH + extraOffset;
        else if (chstr[0] == SpecialChars::BLANK)
			CurX += glyphs->wide()+itemRenderText.charStyle(a).fontSize() * itemRenderText.charStyle(a).tracking() / 10000.0 + wordExtra + extraOffset;
		else
			CurX += glyphs->wide()+itemRenderText.charStyle(a).fontSize() *itemRenderText.charStyle(a).tracking() / 10000.0 + extraOffset;
	}
}

bool PageItem_PathText::createInfoGroup(QFrame *infoGroup, QGridLayout *infoGroupLayout)
{
	int Parag = 0;
	int Words = 0;
	int Chara = 0;
	int ParagN = 0;
	int WordsN = 0;
	int CharaN = 0;
	
	QLabel *infoCT = new QLabel(infoGroup);
	QLabel *linesCT = new QLabel(infoGroup);
	QLabel *linesT = new QLabel(infoGroup);
	QLabel *parCT = new QLabel(infoGroup);
	QLabel *parT = new QLabel(infoGroup);
	QLabel *wordCT = new QLabel(infoGroup);
	QLabel *wordT = new QLabel(infoGroup);
	QLabel *charCT = new QLabel(infoGroup);
	QLabel *charT = new QLabel(infoGroup);
	
	
	infoCT->setText("Text on a Path");
	infoGroupLayout->addWidget( infoCT, 0, 0, 1, 2, Qt::AlignCenter );
	
	WordAndPara(this, &Words, &Parag, &Chara, &WordsN, &ParagN, &CharaN);
	parCT->setText(tr("Paragraphs: "));
	infoGroupLayout->addWidget( parCT, 1, 0, Qt::AlignRight );
	if (ParagN != 0)
		parT->setText(QString::number(Parag+ParagN)+" ("+QString::number(ParagN)+")");
	else
		parT->setText(QString::number(Parag));
	infoGroupLayout->addWidget( parT, 1, 1 );
	
	linesCT->setText(tr("Lines: "));
	infoGroupLayout->addWidget( linesCT, 2, 0, Qt::AlignRight );
	linesT->setText(QString::number(textLayout.lines()));
	infoGroupLayout->addWidget( linesT, 2, 1 );
	
	
	wordCT->setText(tr("Words: "));
	infoGroupLayout->addWidget( wordCT, 3, 0, Qt::AlignRight );
	if (WordsN != 0)
		wordT->setText(QString::number(Words+WordsN)+" ("+QString::number(WordsN)+")");
	else
		wordT->setText(QString::number(Words));
	infoGroupLayout->addWidget( wordT, 3, 1 );
	
	charCT->setText(tr("Chars: "));
	infoGroupLayout->addWidget(charCT, 4, 0, Qt::AlignRight );
	if (CharaN != 0)
		charT->setText(QString::number(Chara+CharaN)+" ("+QString::number(CharaN)+")");
	else
		charT->setText(QString::number(Chara));
	infoGroupLayout->addWidget( charT, 4, 1 );
	return true;
}

void PageItem_PathText::applicableActions(QStringList & actionList)
{
	actionList << "toolsEditWithStoryEditor";
	actionList << "itemConvertToOutlines";
}

QString PageItem_PathText::infoDescription()
{
	return QString();
}

void PageItem_PathText::getVisualBoundingRect(double * x1, double * y1, double * x2, double * y2) const
{
	PageItem::getVisualBoundingRect(x1, y1, x2, y2);
	QRectF totalRect(QPointF(*x1, *y1), QPointF(*x2, *y2));
	QTransform clipTrans;
	clipTrans.translate(m_xPos, m_yPos);
	clipTrans.rotate(m_rotation);
	totalRect = totalRect.united(QRectF(clipTrans.mapRect(Clip.boundingRect())));
	totalRect.getCoords(x1, y1, x2, y2);
}
