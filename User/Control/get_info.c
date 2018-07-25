#include "get_info.h"



void ReadEncoder(void)
{
	int16_t nEncoder1,nEncoder2,nEncoder3;
	//��ȡCNT��ֵ������
	nEncoder1 = TIM2->CNT;
	TIM_SetCounter(TIM2,0);
	
	nEncoder2 =TIM4->CNT;
	TIM_SetCounter(TIM4,0);
	
	nEncoder3 = TIM8->CNT;
	TIM_SetCounter(TIM8,0);
	
	BasketballRobot.w[2] = nEncoder3;
	BasketballRobot.w[1] = nEncoder2;
	BasketballRobot.w[0] = nEncoder1;  
}

//���봮���ж���
void receiveIMUData(void)
{
	uint8_t sum=0,i=0,res;
	
	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)  //�����ж�(���յ������ݱ�����0x0d 0x0a��β)
	{
		res =USART_ReceiveData(USART2);//(USART2->DR);	//��ȡ���յ�������
		if((USART2_RX_STA&0x8000)==0)//����δ���
		{
			USART2_RX_BUF[USART2_RX_STA&0X3FFF] = res;
			
			if((USART2_RX_STA&0X3FFF)==0&&USART2_RX_BUF[0]!=0x55) return;     //�� 0 �����ݲ���֡ͷ������ 
			if((USART2_RX_STA&0X3FFF)==1&&USART2_RX_BUF[1]!=0x53) return;     //�� 2 �����ݲ��ǽǶȣ����� 
			
			USART2_RX_STA++;
			
			if((USART2_RX_STA&0X3FFF) == 11)
			{
				for(i=0;i<10;i++)
					sum += USART2_RX_BUF[i];
				
				if(sum == USART2_RX_BUF[10])
					USART2_RX_STA|=0x8000;				
			}
		}		
	}					
}

u8 GetYaw(void)
{
	//receiveIMUData();
		
	if(USART2_RX_STA&0x8000)	
	{
		//(Re_buf [7]<<8| Re_buf [6]))/32768.0*180; 
		BasketballRobot.ThetaD = ((float)((USART2_RX_BUF[7]<<8)|USART2_RX_BUF[6]))/32768*180;
		
		BasketballRobot.ThetaR = BasketballRobot.ThetaD * PI / 180;
		
		while(BasketballRobot.ThetaR < 0)
				BasketballRobot.ThetaR  = BasketballRobot.ThetaR + PI + PI;
			
		while (BasketballRobot.ThetaR > 2 * PI)
			BasketballRobot.ThetaR = BasketballRobot.ThetaR - PI - PI;
			
		while(BasketballRobot.ThetaD < 0)
			BasketballRobot.ThetaD  = BasketballRobot.ThetaD + 360;
			
		while (BasketballRobot.ThetaD >360)
			BasketballRobot.ThetaD = BasketballRobot.ThetaD - 360;
		
		USART2_RX_STA=0;
		
		//printf("yaw: %.2f   tim : %d \r\n    ",BasketballRobot.ThetaD, htim5.Instance->CNT);
		LED0 = !LED0;
		LED1 = !LED0;
		
		SendToPc(1,BasketballRobot.X,BasketballRobot.Y,BasketballRobot.ThetaD);
		return 1;
	}
	else 
		return 0;
}


//����ת��,��̼ƶ�λ
void GetPosition(void)
{
	//�����ٶ�����������

	float nW,nX,nY;
	

	float l1,l2,l3;	//��̼Ƽ�ȥ����ƫ�����ֵ
	
	float theta_inv[2][2]; //�ǶȾ���
	
	//GetYaw();
	ReadEncoder();
	
	BasketballRobot.LastTheta = BasketballRobot.ThetaR;
	
	
	//theta_inv
	theta_inv[0][0]= cos(BasketballRobot.ThetaR);	theta_inv[0][1] = -theta_inv[1][0];		
	theta_inv[1][0]= sin(BasketballRobot.ThetaR);	theta_inv[1][1] = theta_inv[0][0];	
	
	nW = (BasketballRobot.w[0]+BasketballRobot.w[1]+BasketballRobot.w[2])/3.0f;	
	//��ȥ�Դ�ƫ��
	l1 = BasketballRobot.w[0] - nW;	
	l2 = BasketballRobot.w[1] - nW;
	l3 = BasketballRobot.w[2] - nW;
	
	nX = -l1/20000;
	nY = -(-l2 + l3)/1.7320508f/22400;
	
	//nX = 
	
	BasketballRobot.X += nX*theta_inv[0][0]+nY*theta_inv[0][1];
	BasketballRobot.Y += nX*theta_inv[1][0]+nY*theta_inv[1][1];
	
	
}

void receiveVisionData(void)
{
	uint8_t sum=0,i=0,res;
	
	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)  //�����ж�(���յ������ݱ�����0x0d 0x0a��β)
	{
		res =USART_ReceiveData(USART1);//(USART1->DR);	//��ȡ���յ�������
		if((Vision.RX_STA&0x8000)==0)//����δ���
		{
			Vision.RX_BUF[Vision.RX_STA&0X3FFF] = res;
			
			//����ʧ��,����λ���ǽ���λ z
			if((Vision.RX_STA&0X3FFF)!=7&&USART2_RX_BUF[Vision.RX_STA&0X3FFF] == 'z') 
			{
				Vision.RX_STA =0;
				Vision.RX_STA |= 0x4000;
				return;
			}
			if(Vision.RX_STA&0x4000)
			{
				Vision.RX_STA =0;
				return;
			}

			Vision.RX_STA++;

			if((Vision.RX_STA&0X3FFF) == 9)
			{
				for(i=0;i<8;i++)
					sum += Vision.RX_BUF[i];
				if(sum == 'z')
					sum += '1';
				if(sum == Vision.RX_BUF[8])
				{
					Vision.RX_STA|=0x8000;	
				}
			}
		}
		
	} 
}

//�Ӿ����ݴ���
u8 GetVisionData(void)
{	
	u32 x,d;

	//receiveVisionData();

	if (Vision.RX_STA&0x8000)
	{
		//����λ����Ϣ
		if (Vision.RX_BUF[0] != ' ')
			x = (Vision.RX_BUF[0] - '0') * 100;
		else
			x = 0;
		Vision.RX_BUF[0] = ' ';

		if (Vision.RX_BUF[1] != ' ')
			x += (Vision.RX_BUF[1] - '0') * 10;
		Vision.RX_BUF[1] = ' ';

		if (Vision.RX_BUF[2] != ' ')
			x += (Vision.RX_BUF[2] - '0');
		Vision.RX_BUF[2] = ' ';

		//�����Ϣ
		if (Vision.RX_BUF[3] != ' ')
			d = (Vision.RX_BUF[3] - '0') * 1000;
		else
			d = 0;
		Vision.RX_BUF[3] = ' ';

		if (Vision.RX_BUF[4] != ' ')
			d += (Vision.RX_BUF[4] - '0') * 100;
		Vision.RX_BUF[4] = ' ';

		if (Vision.RX_BUF[5] != ' ')
			d += (Vision.RX_BUF[5] - '0') * 10;
		Vision.RX_BUF[5] = ' ';

		if (Vision.RX_BUF[6] != ' ')
			d += (Vision.RX_BUF[6] - '0');
		Vision.RX_BUF[6] = ' ';

		Vision.RX_STA = 0;
	}

	if (x < 10 || x > 630 || d < 500)
	{
		Vision.State = 0;
		return 0;
	}

	else
	{
		Vision.X = x;
		Vision.Depth = d;
		Vision.State = 1;
		LCD_ShowString(30 + 200, 420, 200, 16, 16, "View :pix");
		LCD_ShowNum(30 + 200 + 48 + 8 + 45, 420, Vision.X, 4, 16);
		LCD_ShowString(30 + 200, 440, 200, 16, 16, "View :length");
		LCD_ShowNum(30 + 200 + 48 + 8 + 45, 440, Vision.Depth, 4, 16);
		return 1;
	}
}

void receiveRadarData(void)
{
	uint8_t sum=0,i=0,res;
#if 0
	if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)  //�����ж�(���յ������ݱ�����0x0d 0x0a��β)
	{
		res =USART_ReceiveData(USART3);//(USART3->DR);	//��ȡ���յ�������
		if((Radar.RX_STA&0x8000)==0)//����δ���
		{
			Radar.RX_BUF[Radar.RX_STA&0X3FFF] = res;

			if((Radar.RX_STA&0X3FFF)!=7&&USART2_RX_BUF[7] == 'z') 
			{
				Radar.RX_STA = 0;
				return; 
			}
			
			Radar.RX_STA++;

			if((Radar.RX_STA&0X3FFF) == 9)
			{
				for(i=0;i<8;i++)
					sum += Radar.RX_BUF[i];
				// if(sum == 'z')
				// 	sum += '1';
				if(sum = Radar.RX_BUF[8])
				{
					Radar.RX_STA|=0x8000;	
				}
			}
		}
		
	}
#else
	res = USART_ReceiveData(USART3);  //(USART3->DR);	//��ȡ���յ�������
	if ((Radar.RX_STA & 0x8000) == 0) //����δ���
	{
		Radar.RX_BUF[Radar.RX_STA & 0X3FFF] = res;

		if ((Radar.RX_STA & 0X3FFF) == 0 && Radar.RX_BUF[0] != '@')
			return;
		if ((Radar.RX_STA & 0X3FFF) == 1 && Radar.RX_BUF[1] != '^')
			return;
		if ((Radar.RX_STA & 0X3FFF) == 2 && Radar.RX_BUF[2] != 'r')
			return;

		Radar.RX_STA++;

		if((Radar.RX_STA & 0X3FFF) == 10)
		{
			for (i = 0; i < 9; i++)
				sum += Radar.RX_BUF[i];
			if (sum == Radar.RX_BUF[9])
				Radar.RX_STA |= 0x8000;
		}
	}

#endif

}

//���⴦������
u8 GetRadarData(void)
{
	#if 0
{
	if(Radar.RX_STA&0x8000)
	{					   
		//�õ��˴ν��յ������ݳ���
		//len=USART1_RX_STA&0x3fff;		
		//������Ϣ
		if(Radar.RX_BUF[0]!=' ')
			Radar.Distance=(Radar.RX_BUF[0]-'0')*1000;
		else 
			Radar.Distance=0;
		Radar.RX_BUF[0] = ' ';
		
		if(Radar.RX_BUF[1]!=' ')
			Radar.Distance+=(Radar.RX_BUF[1]-'0')*100;
		Radar.RX_BUF[1] = ' ';
		
		if(Radar.RX_BUF[2]!=' ')
			Radar.Distance+=(Radar.RX_BUF[2]-'0')*10;
		Radar.RX_BUF[2] = ' ';
		
		if(Radar.RX_BUF[3]!=' ')
			Radar.Distance +=(Radar.RX_BUF[3]-'0');
		Radar.RX_BUF[3] = ' ';
		
		//�Ƕ���Ϣ
		if(Radar.RX_BUF[4]!=' ')
			Radar.Angle=(Radar.RX_BUF[4]-'0')*100;
		else 
			Radar.Angle=0;
		Radar.RX_BUF[4] = ' ';
		if(Radar.RX_BUF[5]!=' ')
			Radar.Angle+=(Radar.RX_BUF[5]-'0')*10;
		Radar.RX_BUF[5] = ' ';
		if(Radar.RX_BUF[6]!=' ')
			Radar.Angle+=(Radar.RX_BUF[6]-'0');
		Radar.RX_BUF[6] = ' ';

		LCD_ShowString(30+200,460,200,16,16,"Radar:rad");	
		LCD_ShowNum(30+200+48+8+45,460,Radar.Angle,4,16);		
		LCD_ShowString(30+200,480,200,16,16,"Radar:length");	
		LCD_ShowNum(30+200+48+8+45,480,Radar.Distance,4,16);	
		
		Radar.RX_STA=0;
	}
	
	if(Radar.Angle<240 || Radar.Angle >300||Radar.Distance>4000) //ԭ��&&
	{
		Radar.State = 0;
		return 0;
	}	

	else
	{
		Radar.State = 1;
		return 1;
	}	
}
	#else
	u32 a,d;
	
	if(Radar.RX_STA&0x8000)
	{
		a = (Radar.RX_BUF[3]<<8)|Radar.RX_BUF[4];
		d = (Radar.RX_BUF[5]<<8)|Radar.RX_BUF[6];
		Radar.RX_STA=0;
	}
	if(a<240 || a >300||d>4000||d<10) //ԭ��&&
	{
		Radar.State = 0;
		return 0;
	}	
	else
	{
		Radar.Angle = a;
		Radar.Distance = d;
		Radar.State = 1;
		return 1;
	}
	#endif
}
