#include "misc.h"

#ifdef WIN32
#include <Windows.h>
#else
#include <QKeyEvent>
//#include <X11/XKBlib.h>
//# undef KeyPress
//# undef KeyRelease
//# undef FocusIn
//# undef FocusOut
//// #undef those Xlib #defines that conflict with QEvent::Type enum
#endif //zxlwin

#include <QFileInfo>
#include <QDir>
#include <QApplication>
#include <QTextCodec>

template <class key, class value>
key getKeyByValue(QMap<key, value> m, value v)
{
	foreach(key k, m)
	{
		if (m.value(k) == v)   return k;
	}

	return m.end();
}

QString toThousandFigure(int number) // 转换为001,015 这种数字格式
{
	if (number <= 9999 && number >= 1000) {
		return QString::number(number);
	}

	if (number <= 999 && number >= 100) {
		return QString("0" + QString::number(number));
	}

	if (number <= 99 && number >= 10) {
		return QString("00" + QString::number(number));
	}

	if (number <= 9 && number >= 0) {
		return QString("000" + QString::number(number));
	}
	return "99999";
}

QString doubleToStr(double number)
{
	QString num = QString::number(number, 'f', 6);
	int pos = num.indexOf('.') + 6;
	QString str = num.mid(0, pos);

	for (int i = str.length() - 1; i > 0; i--)
	{
		if (str[i] == QChar('0'))
		{
			str = str.left(str.length() - 1);
		}
		else
		{
			if (str[i] == QChar('.'))
			{
				str = str.left(str.length() - 1);
			}
			break;
		}
	}

	return str;
}

QString checkZero(double balance)
{
    QString balanceResult;
    if (balance < 0.00000001 && balance > -0.00000001)
    {
        balanceResult = "0";
    }
    else
    {
		balanceResult = doubleToStr(balance);
    }
    return balanceResult;
}

bool Misc::isInContracts(QString filePath)
{
	QDir dir("contracts");
	QFileInfo fileInfo(filePath);
	return (dir.absolutePath() + "/" + fileInfo.fileName() == filePath);
}

bool Misc::isInScripts(QString filePath) {
	QDir dir("scripts");
	QFileInfo fileInfo(filePath);
	return (dir.absolutePath() + "/" + fileInfo.fileName() == filePath);
}

QString Misc::changePathFormat(QString path) {
	path.replace("\\", "?");
	path.replace("/", "?");
	return path;
}

QString Misc::restorePathFormat(QString path) {
	path.replace("?", "/");
	return path;
}

qint64 Misc::write(QString cmd, QProcess* process) {
	if (!process) {
		return 0;
	}

	QTextCodec* gbkCodec = QTextCodec::codecForName("GBK");
	QByteArray cmdBa = gbkCodec->fromUnicode(cmd);
	process->readAll();
	return process->write(cmdBa.data());
}

QString Misc::read(QProcess* process) {
	QTextCodec* gbkCodec = QTextCodec::codecForName("GBK");
	QString result;
	QString str;
	QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
	while (!result.contains(">>>")) {
		process->waitForReadyRead(50);
		str = gbkCodec->toUnicode(process->readAll());
		result += str;
		if (str.right(2) == ": ")  break;
	}

	QApplication::restoreOverrideCursor();
	return result;
}

QString Misc::readAll(QProcess *process) {
	QTextCodec* gbkCodec = QTextCodec::codecForName("GBK");
	QString result = gbkCodec->toUnicode(process->readAll());

	return result;
}

bool getCapslockState()
{
#ifdef WIN32
    return GetKeyState(VK_CAPITAL) == 1;
#else
//    Qt::KeyboardModifiers info = QKeyEvent::modifiers();;
//    info.isKeyLocked(Qt::Key_CapsLock);
#endif
}

