#include "InputComment.h"
#include "DebuggedProcess.h"
#include <string>
InputComment::InputComment(QDialog * parent, ResultsWindow * pResultsWindow, QTreeWidgetItem * itm, int column) : QDialog(parent), _pResultsWindow(pResultsWindow)
{
	ui.setupUi(this, itm, column);
	QPalette Pal(palette());
	this->setAutoFillBackground(true);
	this->setPalette(Pal);
	this->show();
	_itm = itm;
	_column = column;
	QObject::connect(ui.buttonBox, &QDialogButtonBox::accepted, this, &InputComment::confirm);
	QObject::connect(ui.buttonBox, &QDialogButtonBox::rejected, this, &InputComment::decline);
}
void InputComment::confirm()
{
	switch (_column)
	{
	case 0:
		_itm->setText(_column, ui.textEdit->text());
		this->close();
		break;
	case 2:
		size_t p = 4;
		//size_t *i = &p;
		QString text = ui.textEdit->text();
		uint64_t value = text.toULongLong(0, 16);
		uint64_t addr = std::stoull((_itm->text(1)).toStdString(), &p, 16);
		//call write value
		bool o = WriteTarget(value, addr, 4);
		DRIVOUT << "value " << value << "addr " << addr << "bool " << o << std::endl;
		_itm->setText(_column, ui.textEdit->text());
		this->close();
		break;
	}
}
void InputComment::decline()
{
	this->close();
}
InputComment::~InputComment()
{

}