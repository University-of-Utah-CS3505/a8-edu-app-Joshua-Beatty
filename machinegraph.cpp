#include "machinegraph.h"
#include <QEvent>
#include <QLine>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

#include <QPen>
#include <tuple>
#include <vector>
MachineGraph::MachineGraph(QWidget *parent) : QWidget{parent} {
  blockTree.push_back(-1);
  this->setAttribute(Qt::WA_Hover, true);

  map[0] = std::tuple<ProgramBlock, QPointF, QPoint>(
      ProgramBlock::begin,
      QPointF(this->width() / 2 - GENERAL_BLOCK_SIZE_X / 2,
              this->height() / 2 - GENERAL_BLOCK_SIZE_Y / 2),
      QPoint(GENERAL_BLOCK_SIZE_X, GENERAL_BLOCK_SIZE_Y));
  type = ProgramBlock::moveForward;
  selectedBlock = -1;
  hoverBlock = -1;
  errorBlock = -1;
  connecting = false;
  update();
}

void MachineGraph::paintEvent(QPaintEvent *event) {
  QPainter painter(this);

  // Draw backgournd.
  painter.drawRect(QRect(0, 0, this->width() - 10, this->height() - 10));

  for (const auto &[key, value] : map) {
    bool visited[blockTree.size()];

    for (auto i = 0; i < blockTree.size(); i++) {
      visited[i] = false;
    }

    painter.setPen(QPen(QColor::fromRgb(0, 0, 0), 2, Qt::SolidLine,
                        Qt::RoundCap, Qt::RoundJoin));

    int currentBlock = key;
    while (currentBlock != -1) {
      int next = blockTree[currentBlock];
      if (!visited[currentBlock]) {
        QPointF startPoint = std::get<QPointF>(map[currentBlock]);
        QPointF startSize = std::get<QPoint>(map[currentBlock]);
        visited[currentBlock] = true;
        if (next != -1) {
          ProgramBlock nextType = std::get<ProgramBlock>(map[next]);

          QPoint endSize = std::get<QPoint>(map[next]);
          QPointF endPoint = std::get<QPointF>(map[next]);
          drawConnection(nextType, startPoint + startSize / 2,
                         endPoint + endSize / 2, endSize, painter);
        }
      }
      drawBlock(currentBlock, painter);
      currentBlock = next;
    }
  }
}

void MachineGraph::drawBlock(int blockID, QPainter &painter) {
  QPointF startPoint = std::get<QPointF>(map[blockID]);
  ProgramBlock type = std::get<ProgramBlock>(map[blockID]);
  QPoint size = std::get<QPoint>(map[blockID]);

  bool lighter = false;

  int midY = startPoint.y() + size.y() / 2;
  int midX = startPoint.x() + size.x() / 2;

  QColor blockColor;
  QColor innerBlockColor;

  if (blockID == hoverBlock || blockID == selectedBlock) {
    lighter = true;
  }
  if (blockID == hoverBlock && blockID == selectedBlock) {
    lighter = false;
  }

  if (type == ProgramBlock::begin) {
    blockColor = beginBlockColor;

  } else if (type == ProgramBlock::ifStatement || type == ProgramBlock::endIf) {
    blockColor = ifBlockColor;

  } else if (type == ProgramBlock::whileLoop ||
             type == ProgramBlock::endWhile) {
    blockColor = whileBlockColor;

  } else {
    blockColor = generalBlockColor;
  }

  // Draw error message
  if (errorBlock == blockID) {
    blockColor = errorBlockColor;
    painter.setPen(errorBlockColor);
    painter.drawLine(QLine(startPoint.x() + size.x(), midY,
                           startPoint.x() + size.x() + 20, midY));
    painter.drawText(startPoint.x() + size.x() + 30, midY + 5,
                     errorMessage.c_str());
  }
  painter.setPen(Qt::black);
  if (lighter) {
    blockColor = blockColor.lighter();
    innerBlockColor = innerBlockColor.lighter();
  }
  innerBlockColor = blockColor.lighter().lighter();

  QColor borderAndTextColorOuter;
  QColor borderAndTextColorInner;

  if (blockColor.lightness() > 150) {
    borderAndTextColorOuter = Qt::black;
  } else {
    borderAndTextColorOuter = Qt::white;
  }

  if (innerBlockColor.lightness() > 150) {
    borderAndTextColorInner = Qt::black;
  } else {
    borderAndTextColorInner = Qt::white;
  }

  QPen outerPen(borderAndTextColorOuter, 2);
  QPen innerPen(borderAndTextColorInner, 2);

  switch (type) {
  case ProgramBlock::whileLoop: {
  }
  case ProgramBlock::ifStatement: {
    QPainterPath path;
    int firstConstGap = 50;
    int secondConstGap = 10;
    path.addRoundedRect(QRectF(startPoint.x(), startPoint.y(),
                               CONDITIONAL_BLOCK_SIZE_X,
                               CONDITIONAL_BLOCK_SIZE_Y),
                        5, 5);
    painter.setPen(outerPen);
    painter.fillPath(path, blockColor);
    painter.drawPath(path);
    drawTextFromMid(QPointF(startPoint.x() + firstConstGap / 2, midY + 5),
                    this->getText(type), painter);

    ProgramBlock firstConst = std::get<0>(condition[blockID]);
    ProgramBlock secondConst = std::get<1>(condition[blockID]);

    QPainterPath pathInner1;
    QPainterPath pathInner2;
    painter.setPen(innerPen);

    pathInner1.addRoundedRect(QRectF(startPoint.x() + firstConstGap,
                                     midY - INNER_BLOCK_SIZE_SMALLER_Y / 2,
                                     INNER_BLOCK_SIZE_SMALLER_X,
                                     INNER_BLOCK_SIZE_SMALLER_Y),
                              5, 5);

    painter.fillPath(pathInner1, innerBlockColor);
    drawTextFromMid(
        QPointF(startPoint.x() + firstConstGap + INNER_BLOCK_SIZE_SMALLER_X / 2,
                midY + 5),
        this->getText(firstConst), painter);

    pathInner2.addRoundedRect(QRectF(startPoint.x() + firstConstGap +
                                         secondConstGap +
                                         INNER_BLOCK_SIZE_SMALLER_X,
                                     midY - INNER_BLOCK_SIZE_Y / 2,
                                     INNER_BLOCK_SIZE_X, INNER_BLOCK_SIZE_Y),
                              5, 5);
    painter.fillPath(pathInner2, innerBlockColor);
    drawTextFromMid(QPointF(startPoint.x() + firstConstGap +
                                INNER_BLOCK_SIZE_SMALLER_X + secondConstGap +
                                INNER_BLOCK_SIZE_X / 2,
                            midY + 5),
                    this->getText(secondConst), painter);
    break;
  }
    //  case ProgramBlock::whileLoop: {
    //    painter.fillRect(startPoint.x(), startPoint.y(),
    //    CONDITIONAL_BLOCK_SIZE_X,
    //                     CONDITIONAL_BLOCK_SIZE_Y, blockColor);
    //    painter.drawText(startPoint.x() + 5, midY,
    //    this->getText(type).c_str()); ProgramBlock firstConst =
    //    std::get<0>(condition[blockID]); ProgramBlock secondConst =
    //    std::get<1>(condition[blockID]); painter.fillRect(startPoint.x() + 50,
    //    midY - INNER_BLOCK_SIZE_SMALLER_Y / 2,
    //                     INNER_BLOCK_SIZE_SMALLER_X,
    //                     INNER_BLOCK_SIZE_SMALLER_Y, innerBlockColor);
    //    painter.drawText(startPoint.x() + 50, midY,
    //                     this->getText(firstConst).c_str());

    //    painter.fillRect(startPoint.x() + 60 + INNER_BLOCK_SIZE_SMALLER_X,
    //                     midY - INNER_BLOCK_SIZE_Y / 2, INNER_BLOCK_SIZE_X,
    //                     INNER_BLOCK_SIZE_Y, innerBlockColor);
    //    painter.drawText(startPoint.x() + 60 + INNER_BLOCK_SIZE_SMALLER_X,
    //    midY,
    //                     this->getText(secondConst).c_str());
    //    break;
    //  }
  default: {
    QPainterPath path;
    path.addRoundedRect(
        QRectF(startPoint.x(), startPoint.y(), size.x(), GENERAL_BLOCK_SIZE_Y),
        5, 5);
    painter.setPen(outerPen);
    painter.fillPath(path, blockColor);
    painter.drawPath(path);
    drawTextFromMid(QPointF(midX, midY + 5), this->getText(type), painter);
  }
  }
}

void MachineGraph::drawConnection(ProgramBlock type, QPointF start, QPointF end,
                                  QPoint size, QPainter &painter) {

  painter.setPen(Qt::black);
  float distX = qAbs(end.x() - start.x());
  float distY = qAbs(end.y() - start.y());
  int arrowSize = 5;
  if (type == ProgramBlock::endIf || type == ProgramBlock::endWhile) {
    if (distY < distX) {
      painter.drawLine(QLine(start.x(), start.y(), start.x(), end.y()));
      painter.drawLine(QLine(start.x(), end.y(), end.x(), end.y()));
      if (end.x() < start.x()) {
        QPainterPath path;
        path.moveTo(end.x() + size.x() / 2, end.y());
        path.lineTo(end.x() + size.x() / 2 + arrowSize, end.y() - arrowSize);
        path.lineTo(end.x() + size.x() / 2 + arrowSize, end.y() + arrowSize);
        painter.fillPath(path, QColor::fromRgb(0, 0, 0));
      } else {
        QPainterPath path;
        path.moveTo(end.x() - size.x() / 2, end.y());
        path.lineTo(end.x() - size.x() / 2 - arrowSize, end.y() - arrowSize);
        path.lineTo(end.x() - size.x() / 2 - arrowSize, end.y() + arrowSize);
        painter.fillPath(path, QColor::fromRgb(0, 0, 0));
      }
    } else {
      painter.drawLine(QLine(start.x(), start.y(), end.x(), start.y()));
      painter.drawLine(QLine(end.x(), start.y(), end.x(), end.y()));
      if (end.y() < start.y()) {
        QPainterPath path;
        path.moveTo(end.x(), end.y() + size.y() / 2);
        path.lineTo(end.x() + arrowSize, end.y() + size.y() / 2 + arrowSize);
        path.lineTo(end.x() - arrowSize, end.y() + size.y() / 2 + arrowSize);
        painter.fillPath(path, QColor::fromRgb(0, 0, 0));
      } else {
        QPainterPath path;
        path.moveTo(end.x(), end.y() - size.y() / 2);
        path.lineTo(end.x() + arrowSize, end.y() - size.y() / 2 - arrowSize);
        path.lineTo(end.x() - arrowSize, end.y() - size.y() / 2 - arrowSize);
        painter.fillPath(path, QColor::fromRgb(0, 0, 0));
      }
    }
    return;
  }

  if (distY > distX) {
    painter.drawLine(QLine(start.x(), start.y(), start.x(), end.y()));
    painter.drawLine(QLine(start.x(), end.y(), end.x(), end.y()));
    // Arrow points left.
    if (start.x() > end.x() - size.x() / 2 &&
        start.x() < end.x() + size.x() / 2) {
      if (end.y() > start.y()) {
        QPainterPath path;
        path.moveTo(start.x(), end.y() - size.y() / 2);
        path.lineTo(start.x() - arrowSize, end.y() - size.y() / 2 - arrowSize);
        path.lineTo(start.x() + arrowSize, end.y() - size.y() / 2 - arrowSize);
        painter.fillPath(path, QColor::fromRgb(0, 0, 0));
      } else {
        QPainterPath path;
        path.moveTo(start.x(), end.y() + size.y() / 2);
        path.lineTo(start.x() - arrowSize, end.y() + size.y() / 2 + arrowSize);
        path.lineTo(start.x() + arrowSize, end.y() + size.y() / 2 + arrowSize);
        painter.fillPath(path, QColor::fromRgb(0, 0, 0));
      }
      return;
    }
    if (end.x() < start.x()) {
      QPainterPath path;
      path.moveTo(end.x() + size.x() / 2, end.y());
      path.lineTo(end.x() + size.x() / 2 + arrowSize, end.y() - arrowSize);
      path.lineTo(end.x() + size.x() / 2 + arrowSize, end.y() + arrowSize);
      painter.fillPath(path, QColor::fromRgb(0, 0, 0));
    } else {
      QPainterPath path;
      path.moveTo(end.x() - size.x() / 2, end.y());
      path.lineTo(end.x() - size.x() / 2 - arrowSize, end.y() - arrowSize);
      path.lineTo(end.x() - size.x() / 2 - arrowSize, end.y() + arrowSize);
      painter.fillPath(path, QColor::fromRgb(0, 0, 0));
    }
  } else {
    painter.drawLine(QLine(start.x(), start.y(), end.x(), start.y()));
    painter.drawLine(QLine(end.x(), start.y(), end.x(), end.y()));

    if (start.y() > end.y() - size.y() / 2 &&
        start.y() < end.y() + size.y() / 2) {
      if (end.x() > start.x()) {
        QPainterPath path;
        path.moveTo(end.x() - size.x() / 2, start.y());
        path.lineTo(end.x() - size.x() / 2 - arrowSize, start.y() - arrowSize);
        path.lineTo(end.x() - size.x() / 2 - arrowSize, start.y() + arrowSize);
        painter.fillPath(path, QColor::fromRgb(0, 0, 0));
      } else {
        QPainterPath path;
        path.moveTo(end.x() + size.x() / 2, start.y());
        path.lineTo(end.x() + size.x() / 2 + arrowSize, start.y() - arrowSize);
        path.lineTo(end.x() + size.x() / 2 + arrowSize, start.y() + arrowSize);
        painter.fillPath(path, QColor::fromRgb(0, 0, 0));
      }
      return;
    }
    if (end.y() < start.y()) {
      QPainterPath path;
      path.moveTo(end.x(), end.y() + size.y() / 2);
      path.lineTo(end.x() + arrowSize, end.y() + size.y() / 2 + arrowSize);
      path.lineTo(end.x() - arrowSize, end.y() + size.y() / 2 + arrowSize);
      painter.fillPath(path, QColor::fromRgb(0, 0, 0));
    } else {
      QPainterPath path;
      path.moveTo(end.x(), end.y() - size.y() / 2);
      path.lineTo(end.x() + arrowSize, end.y() - size.y() / 2 - arrowSize);
      path.lineTo(end.x() - arrowSize, end.y() - size.y() / 2 - arrowSize);
      painter.fillPath(path, QColor::fromRgb(0, 0, 0));
    }
  }
}

void MachineGraph::drawTextFromMid(QPointF position, std::string text,
                                   QPainter &painter) {
  QFontMetrics fm(painter.font());
  const std::string str = text;
  int xoffset = fm.boundingRect(str.c_str()).width() / 2;
  painter.drawText(position.x() - xoffset, position.y(), str.c_str());
}

void MachineGraph::connectBlock(int block1, int block2) {
  if (blockTree[block2] != block1 && block2 != block1) {
    blockTree[block1] = block2;
  }
}

void MachineGraph::toggleConnecting() {
  connecting = !connecting;
  if (connecting)
    setCursor(Qt::CrossCursor);
  else {
    setCursor(Qt::ArrowCursor);
  }
  emit connectToggled(connecting);
}

bool MachineGraph::event(QEvent *event) {
  switch (event->type()) {
  case QEvent::MouseButtonDblClick:
    mouseClickHandler(static_cast<QMouseEvent *>(event));
    return false;
    break;
  case QEvent::MouseButtonPress:
    mousePressHandler(static_cast<QMouseEvent *>(event));
    return false;
    break;
  case QEvent::MouseButtonRelease:
    mouseReleaseHandler(static_cast<QMouseEvent *>(event));
    return false;
    break;
  case QEvent::MouseMove:
    mouseMoveHandler(static_cast<QMouseEvent *>(event));
    return false;
    break;
  default:
    break;
  }
  QWidget::event(event);
  return false;
}
void MachineGraph::mouseMoveHandler(QMouseEvent *event) {
  if (moving && selectedBlock != -1 && !connecting) {
    QPoint size = std::get<QPoint>(map[selectedBlock]);
    std::get<QPointF>(map[selectedBlock]) = event->position() - size / 2;
  }
  if (connecting) {
    int blockId = getBlock(event->position());
    hoverBlock = blockId;
  }
  update();
}
void MachineGraph::mouseReleaseHandler(QMouseEvent *event) {
  if (connecting) {
    int blockId = getBlock(event->position());
    if (blockTree[blockId] != selectedBlock && selectedBlock != -1) {
      connectBlock(selectedBlock, blockId);
    }
  }
  hoverBlock = -1;
  selectedBlock = -1;
  moving = false;
  update();
}
void MachineGraph::mousePressHandler(QMouseEvent *event) {
  int blockId = getBlock(event->position());
  errorBlock = -1;
  if (blockId != -1) {
    selectedBlock = blockId;
  }
  moving = true;
  update();
}

void MachineGraph::mouseClickHandler(QMouseEvent *event) {
  if (Qt::LeftButton && event->position().x() > 0 &&
      event->position().y() > 0) {
    addBlock(type, event->position());
    update();
  }
}
const std::string MachineGraph::getText(ProgramBlock p) {
  switch (p) {
  case ProgramBlock::conditionFacingBlock:
    return "Facing Block";
  case ProgramBlock::conditionNot:
    return "Not";
  case ProgramBlock::conditionFacingPit:
    return "Facing Pit";
  case ProgramBlock::conditionFacingWall:
    return "Facing Wall";
  case ProgramBlock::conditionFacingCheese:
    return "Facing Cheese";
  case ProgramBlock::begin:
    return "Begin";
  case ProgramBlock::blank:
    return "";
  case ProgramBlock::eatCheese:
    return "Eat Chese";
  case ProgramBlock::endIf:
    return "End If";
  case ProgramBlock::endWhile:
    return "End While";
  case ProgramBlock::whileLoop:
    return "While";
  case ProgramBlock::ifStatement:
    return "If";
  case ProgramBlock::turnLeft:
    return "Turn Left";
  case ProgramBlock::turnRight:
    return "Turn Right";
  case ProgramBlock::moveForward:
    return "Move Forward";
  }
}

void MachineGraph::addBlock(ProgramBlock type, QPointF position) {
  if (type == ProgramBlock::conditionFacingBlock ||
      type == ProgramBlock::conditionFacingPit ||
      type == ProgramBlock::conditionFacingWall ||
      type == ProgramBlock::conditionFacingCheese ||
      type == ProgramBlock::conditionNot) {

    int blockId = getBlock(position);
    ProgramBlock blockType = std::get<ProgramBlock>(map[blockId]);
    if (blockType == ProgramBlock::whileLoop ||
        blockType == ProgramBlock::ifStatement) {
      if (type == ProgramBlock::conditionNot) {
        std::get<0>(condition[blockId]) = type;
      } else {
        std::get<1>(condition[blockId]) = type;
      }
    }
    return;
  }
  int id = blockTree.size();
  blockTree.push_back(-1);
  if (type == ProgramBlock::ifStatement || type == ProgramBlock::whileLoop) {
    map[id] = std::tuple<ProgramBlock, QPointF, QPoint>(
        type, position,
        QPoint(CONDITIONAL_BLOCK_SIZE_X, CONDITIONAL_BLOCK_SIZE_Y));
    condition[id] = std::tuple<ProgramBlock, ProgramBlock>(ProgramBlock::blank,
                                                           ProgramBlock::blank);
  } else {
    map[id] = std::tuple<ProgramBlock, QPointF, QPoint>(
        type, position, QPoint(GENERAL_BLOCK_SIZE_X, GENERAL_BLOCK_SIZE_Y));
  }
}

int MachineGraph::getBlock(QPointF point) {
  for (const auto &[key, value] : map) {
    QPointF startPoint = std::get<QPointF>(map[key]);
    QPoint size = std::get<QPoint>(map[key]);
    if (point.x() < (startPoint.x() + size.x()) && point.x() > startPoint.x() &&
        point.y() < (startPoint.y() + size.y()) && point.y() > startPoint.y()) {
      return key;
    }
  }

  return -1;
}
void MachineGraph::setType(ProgramBlock type) { this->type = type; }

void MachineGraph::setErrorMessage(int blockId, std::string message) {
  errorBlock = blockId;
  errorMessage = message;
  update();
}

std::vector<ProgramBlock> MachineGraph::getProgram() {
  int currentBlock = 0;
  std::vector<int> blockId;
  std::vector<ProgramBlock> program;
  std::vector<ProgramBlock> grammaStack;
  while (currentBlock != -1) {
    int next = blockTree[currentBlock];
    ProgramBlock type = std::get<ProgramBlock>(map[currentBlock]);
    program.push_back(type);
    if (type == ProgramBlock::ifStatement || type == ProgramBlock::whileLoop) {
      grammaStack.push_back(type);
      blockId.push_back(currentBlock);
      ProgramBlock firstConst = std::get<0>(condition[currentBlock]);
      ProgramBlock secondConst = std::get<1>(condition[currentBlock]);
      if (secondConst == ProgramBlock::blank) {
        setErrorMessage(currentBlock, "Incomplete conditinal statement");
        return std::vector<ProgramBlock>(ProgramBlock::blank);
      } else {
        program.push_back(firstConst);
        program.push_back(secondConst);
      }
    }

    if (type == ProgramBlock::endIf) {
      if (grammaStack.empty() ||
          grammaStack.back() != ProgramBlock::ifStatement) {
        setErrorMessage(currentBlock, "No matched If statement for End If");
        return std::vector<ProgramBlock>(ProgramBlock::blank);
      } else {
        grammaStack.pop_back();
      }
    }

    if (type == ProgramBlock::endWhile) {
      if (grammaStack.empty() ||
          grammaStack.back() != ProgramBlock::whileLoop) {
        setErrorMessage(currentBlock, "No matched If statement for End While");
        return std::vector<ProgramBlock>(ProgramBlock::blank);
      } else {
        grammaStack.pop_back();
      }
    }
    currentBlock = next;
  }

  if (!grammaStack.empty()) {
    setErrorMessage(blockId.back(), "Needs end statement");
    return std::vector<ProgramBlock>(ProgramBlock::blank);
  }
  qDebug() << program;

  return program;
}