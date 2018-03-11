#pragma once
#include "artefact.h"

//��� ��� ������ ����� CElectricBall, ������� �� ����� ���� ������� ���������� ������� ���� ����������, �� ������ �� �������� ��������
//� ���� � ��� ������ ���� ����� ������� ����������� ��������, �� ������� ���� ����� ������

class C_Arterfact : public CArtefact
{
private:
    typedef CArtefact inherited;
public:
    C_Arterfact(void);
    virtual ~C_Arterfact(void);

    virtual void Load(LPCSTR section);

protected:
    virtual void	UpdateCLChild();

};