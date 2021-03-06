/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "text.h"

#include <QTextDocument>

OLIVE_NAMESPACE_ENTER

enum TextVerticalAlign {
  kVerticalAlignTop,
  kVerticalAlignCenter,
  kVerticalAlignBottom,
};

TextGenerator::TextGenerator()
{
  QString default_str = QStringLiteral("<html><div style='font-size:144pt; text-align: center;'>%1</div></html>").arg(tr("Sample Text"));

  text_input_ = new NodeInput(QStringLiteral("text_in"),
                              NodeParam::kText,
                              default_str);
  AddInput(text_input_);

  color_input_ = new NodeInput(QStringLiteral("color_in"),
                               NodeParam::kColor,
                               QVariant::fromValue(Color(1.0f, 1.0f, 1.0)));
  AddInput(color_input_);

  valign_input_ = new NodeInput(QStringLiteral("valign_in"),
                                NodeParam::kCombo,
                                1);
  AddInput(valign_input_);
}

Node *TextGenerator::copy() const
{
  return new TextGenerator();
}

QString TextGenerator::Name() const
{
  return tr("Text");
}

QString TextGenerator::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.textgenerator");
}

QList<Node::CategoryID> TextGenerator::Category() const
{
  return {kCategoryGenerator};
}

QString TextGenerator::Description() const
{
  return tr("Generate rich text.");
}

void TextGenerator::Retranslate()
{
  text_input_->set_name(tr("Text"));
  color_input_->set_name(tr("Color"));
  valign_input_->set_combobox_strings({tr("Top"), tr("Center"), tr("Bottom")});
}

NodeValueTable TextGenerator::Value(NodeValueDatabase &value) const
{
  GenerateJob job;
  job.InsertValue(text_input_, value);
  job.InsertValue(color_input_, value);
  job.InsertValue(valign_input_, value);
  job.SetAlphaChannelRequired(true);

  NodeValueTable table = value.Merge();

  if (!job.GetValue(text_input_).data().toString().isEmpty()) {
    table.Push(NodeParam::kGenerateJob, QVariant::fromValue(job), this);
  }

  return table;
}

void TextGenerator::GenerateFrame(FramePtr frame, const GenerateJob& job) const
{
  // This could probably be more optimized, but for now we use Qt to draw to a QImage.
  // QImages only support integer pixels and we use float pixels, so what we do here is draw onto
  // a single-channel QImage (alpha only) and then transplant that alpha channel to our float buffer
  // with correct float RGB.
  QImage img(frame->width(), frame->height(), QImage::Format_Grayscale8);
  img.fill(0);

  QTextDocument text_doc;
  text_doc.setHtml(job.GetValue(text_input_).data().toString());
  text_doc.setTextWidth(frame->video_params().width());

  // Draw rich text onto image
  QPainter p(&img);
  p.scale(1.0 / frame->video_params().divider(), 1.0 / frame->video_params().divider());

  TextVerticalAlign valign = static_cast<TextVerticalAlign>(job.GetValue(valign_input_).data().toInt());
  if (valign != kVerticalAlignTop) {
    int doc_height = text_doc.size().height();

    if (valign == kVerticalAlignCenter) {
      // Center align
      p.translate(0, frame->video_params().height() / 2 - doc_height / 2);
    } else {
      // Must be bottom align
      p.translate(0, frame->video_params().height() - doc_height);
    }
  }

  text_doc.drawContents(&p);

  // Transplant alpha channel to frame
  Color rgb = job.GetValue(color_input_).data().value<Color>();
  for (int x=0; x<frame->width(); x++) {
    for (int y=0; y<frame->height(); y++) {
      uchar src_alpha = img.bits()[img.bytesPerLine() * y + x];
      float alpha = float(src_alpha) / 255.0f;

      frame->set_pixel(x, y, Color(rgb.red() * alpha, rgb.green() * alpha, rgb.blue() * alpha, alpha));
    }
  }
}

OLIVE_NAMESPACE_EXIT
