[�����������]
This generator can generate c++ code to blind the script to LongUI app.

in this generator, there are two kinds of scripts, one is you wanted, 
"blind" script, defaulty, impl 2 language: mruby and lua. if you want
to blind other language, you should impl some interface by youself, 
reference the default imp.

and another script, I call it "InterfaceScript", it is a very easy
script like C++, even with called "script". defaulty, you may not
care about it, if you want to blind other method or impl some custom 
control and want to blind to the script, I think you should impl it.

                                MUST SPACE
                                    |
define class:                       |
    [// COMMENT]                    |
    class FULLCLASSNAME[[ALIASNAME]] : SUPERLASSNAME {
        .....
        [Method Zone]
        .....
    };
** if no superclass, just set void
  
define method:
    [// COMMENT]
    [static] RETYPE METHODNAME[[ALIASNAME]](PARAMLIST);

About PARAMLIST:
    No const(and so on), No ref(&), all int32_t, uint32_t...
    is int, just easy, if careful about bit-number or signe,
    even if you want to store some pointer data with size_t,
    as always, and, some ex-type, impl by you self, just easy
    and, NO SPACE with type, hard to deal with space by regex

Example:
    // ABCD
    class MyCo : void {
       // Test
       static void Test(void* a);
       void Test2[test2](void* a);
    };
    class TestContainer[TeCont] : LongUI::UIContainer {
       static void Test[test](void* a);
       // Test2
       void Test2(void* a);
    };

  
����������ܹ�����C++�������ڰ� LongUI Ӧ����ű�����.

���������������, ��Ϊ����ű�, һ��������Ҫ�󶨵Ľű�����, Ĭ���ṩ��
2������: mruby �� lua.�������������ű�����, ���Լ�ʵ��, Ĭ���ṩ��
����Ϊ����Ĳ���

����һ�־����Լ���Ϊ"InterfaceScript"�Ľű�, �����﷨�ǳ��򵥲���������
C++, ���Ҽ򵥵����ܳ�֮Ϊ"�ű�"��. Ĭ�������, ����������. �������Ҫ��
�ӵ���������(Ĭ���ṩ�Ŀ��ܲ���)����˵ʵ�����Զ���Ŀؼ�������Ҫ�󶨵���
��, ��ʱ�����ܾ���Ҫ�˽�����:

                       �����пո�
                          |
����һ����:               |
    [// ע��]             |
    class ��ȫ��[[�����]] : ����ȫ�� {
        .....
        [������]
        .....
    };
û�и������дvoid

����һ������:
    [// ע��]
    [static] �������� ��������[[��������]](�����б�);

���ڲ����б�:
    û��const���޶���, û������(&), �������ζ���int, 
    �򵥾ͺ�, ������ĺܹ���λ�����߷���, ���о�����
    ����size_t����ָ������, ��������, ���Լ�ʵ��.
    ���о�����������Ҫ���ո�, ��������ʽ���Ѵ���

����: �ο������Ӣ�İ��ĩβ