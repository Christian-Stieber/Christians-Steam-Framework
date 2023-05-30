/*
 * This file is part of "Christians-Steam-Framework"
 * Copyright (C) 2023- Christian Stieber
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file LICENSE.  If not see
 * <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "External/MagicEnum/magic_enum.hpp"

/************************************************************************/
/*
 * SteamKit: SteamLanguage.cs -> EPaymentMethod
 */

namespace SteamBot
{
	enum class PaymentMethod : uint32_t
	{
		None = 0,
		ActivationCode = 1,
		CreditCard = 2,
		Giropay = 3,
		PayPal = 4,
		Ideal = 5,
		PaySafeCard = 6,
		Sofort = 7,
		GuestPass = 8,
		WebMoney = 9,
		MoneyBookers = 10,
		AliPay = 11,
		Yandex = 12,
		Kiosk = 13,
		Qiwi = 14,
		GameStop = 15,
		HardwarePromo = 16,
		MoPay = 17,
		BoletoBancario = 18,
		BoaCompraGold = 19,
		BancoDoBrasilOnline = 20,
		ItauOnline = 21,
		BradescoOnline = 22,
		Pagseguro = 23,
		VisaBrazil = 24,
		AmexBrazil = 25,
		Aura = 26,
		Hipercard = 27,
		MastercardBrazil = 28,
		DinersCardBrazil = 29,
		AuthorizedDevice = 30,
		MOLPoints = 31,
		ClickAndBuy = 32,
		Beeline = 33,
		Konbini = 34,
		EClubPoints = 35,
		CreditCardJapan = 36,
		BankTransferJapan = 37,
		PayEasy = 38,
		Zong = 39,
		CultureVoucher = 40,
		BookVoucher = 41,
		HappymoneyVoucher = 42,
		ConvenientStoreVoucher = 43,
		GameVoucher = 44,
		Multibanco = 45,
		Payshop = 46,
		MaestroBoaCompra = 47,
		OXXO = 48,
		ToditoCash = 49,
		Carnet = 50,
		SPEI = 51,
		ThreePay = 52,
		IsBank = 53,
		Garanti = 54,
		Akbank = 55,
		YapiKredi = 56,
		Halkbank = 57,
		BankAsya = 58,
		Finansbank = 59,
		DenizBank = 60,
		PTT = 61,
		CashU = 62,
		AutoGrant = 64,
		WebMoneyJapan = 65,
		OneCard = 66,
		PSE = 67,
		Exito = 68,
		Efecty = 69,
		Paloto = 70,
		PinValidda = 71,
		MangirKart = 72,
		BancoCreditoDePeru = 73,
		BBVAContinental = 74,
		SafetyPay = 75,
		PagoEfectivo = 76,
		Trustly = 77,
		UnionPay = 78,
		BitCoin = 79,
		Wallet = 128,
		Valve = 129,
		MasterComp = 130,
		Promotional = 131,
		MasterSubscription = 134,
		Payco = 135,
		MobileWalletJapan = 136,
		OEMTicket = 256,
		Split = 512,
		Complimentary = 1024,
	};
}

/************************************************************************/

namespace magic_enum
{
	namespace customize
	{
		template <> struct enum_range<SteamBot::PaymentMethod>
		{
			static constexpr int min=0;
			static constexpr int max=1024;
		};
	}
}
