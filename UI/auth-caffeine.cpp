#include "auth-caffeine.hpp"

#include <QFontDatabase>
#include <QMessageBox>

#include <stdlib.h>

#include "window-basic-main.hpp"
#include "window-caffeine.hpp"

#include "ui-config.h"

#include "ui_CaffeineSignIn.h"

static Auth::Def caffeineDef = {"Caffeine", Auth::Type::Custom, true};
const int otpLength = 6;
// Get the Caffeine URL default or staging
static const char *getCaffeineURL = getenv("LIBCAFFEINE_DOMAIN") == NULL
					    ? "caffeine.tv"
					    : getenv("LIBCAFFEINE_DOMAIN");

/* ------------------------------------------------------------------------- */

static int addFonts()
{
	QFontDatabase::addApplicationFont(
		":/caffeine/fonts/Poppins-Regular.ttf");
	QFontDatabase::addApplicationFont(":/caffeine/fonts/Poppins-Bold.ttf");
	QFontDatabase::addApplicationFont(":/caffeine/fonts/Poppins-Light.ttf");
	QFontDatabase::addApplicationFont(
		":/caffeine/fonts/Poppins-ExtraLight.ttf");
	return 0;
}

CaffeineAuth::CaffeineAuth(const Def &d) : OAuthStreamKey(d)
{
	UNUSED_PARAMETER(d);
	instance = caff_createInstance();
}

CaffeineAuth::~CaffeineAuth()
{
	caff_freeInstance(&instance);
}

bool CaffeineAuth::GetChannelInfo()
try {
	key_ = refresh_token;

	if (caff_isSignedIn(instance)) {
		username = caff_getUsername(instance);
	} else {
		switch (caff_refreshAuth(instance, refresh_token.c_str())) {
		case caff_ResultSuccess:
			username = caff_getUsername(instance);
			break;
		case caff_ResultRefreshTokenRequired:
		case caff_ResultInfoIncorrect:
			throw ErrorInfo(Str("Caffeine.Auth.Unauthorized"),
					Str("Caffeine.Auth.IncorrectRefresh"));
		default:
			throw ErrorInfo(Str("Caffeine.Auth.Failed"),
					Str("Caffeine.Auth.SigninFailed"));
		}
	}

	OBSBasic *main = OBSBasic::Get();
	obs_service_t *service = main->GetService();
	obs_data_t *settings = obs_service_get_settings(service);
	obs_data_set_string(settings, "username", username.c_str());
	obs_data_release(settings);

	return true;
} catch (ErrorInfo info) {
	QString title = QTStr("Auth.ChannelFailure.Title");
	QString text = QTStr("Auth.ChannelFailure.Text")
			       .arg(service(), info.message.c_str(),
				    info.error.c_str());

	QMessageBox::warning(OBSBasic::Get(), title, text);

	blog(LOG_WARNING, "%s: %s: %s", __FUNCTION__, info.message.c_str(),
	     info.error.c_str());
	return false;
}

void CaffeineAuth::SaveInternal()
{
	OBSBasic *main = OBSBasic::Get();
	config_set_string(main->Config(), service(), "Username",
			  username.c_str());
	if (uiLoaded) {
		config_set_string(main->Config(), service(), "DockState",
				  main->saveState().toBase64().constData());
	}
	OAuthStreamKey::SaveInternal();
}

static inline std::string get_config_str(OBSBasic *main, const char *section,
					 const char *name)
{
	const char *val = config_get_string(main->Config(), section, name);
	return val ? val : "";
}

bool CaffeineAuth::LoadInternal()
{
	OBSBasic *main = OBSBasic::Get();
	username = get_config_str(main, service(), "Username");
	firstLoad = false;
	return OAuthStreamKey::LoadInternal();
}

class CaffeineChat : public OBSDock {
public:
	inline CaffeineChat() : OBSDock() {}
};

void CaffeineAuth::LoadUI()
{
	if (uiLoaded)
		return;
	if (!GetChannelInfo())
		return;
	/* TODO: Chat */

	// Panel
	panelDock = QSharedPointer<CaffeineInfoPanel>(
			    new CaffeineInfoPanel(this, instance))
			    .dynamicCast<OBSDock>();

	uiLoaded = true;
	return;
}

void CaffeineAuth::OnStreamConfig()
{
	OBSBasic *main = OBSBasic::Get();
	obs_service_t *service = main->GetService();
	obs_data_t *data = obs_service_get_settings(service);
	QSharedPointer<CaffeineInfoPanel> panel2 =
		panelDock.dynamicCast<CaffeineInfoPanel>();
	obs_data_set_string(data, "broadcast_title",
			    panel2->getTitle().c_str());
	obs_data_set_int(data, "rating",
			 static_cast<int64_t>(panel2->getRating()));
	obs_service_update(service, data);
	obs_data_release(data);
}

bool CaffeineAuth::RetryLogin()
{
	std::shared_ptr<Auth> ptr = Login(OBSBasic::Get());
	return ptr != nullptr;
}

void CaffeineAuth::TryAuth(Ui::CaffeineSignInDialog *ui, QDialog *dialog,
			   std::string &passwordForOtp)
{
	std::string username = ui->usernameEdit->text().toStdString();
	std::string otp;
	std::string password;
	if (passwordForOtp.empty()) {
		password = ui->passwordEdit->text().toStdString();
	} else {
		otp = ui->passwordEdit->text().toStdString();
		password = passwordForOtp;
	}

	auto result = caff_signIn(instance, username.c_str(), password.c_str(),
				  otp.c_str());
	switch (result) {
	case caff_ResultSuccess:
		refresh_token = caff_getRefreshToken(instance);
		dialog->accept();
		return;
	case caff_ResultMfaOtpRequired:
		ui->messageLabel->setText(
			QString(R"(%1 <a href="https://www.%2" style="text-decoration:none;color:#009fe0;"></a>)")
				.arg(QTStr("Caffeine.Auth.EnterCode"),
				     getCaffeineURL));
		ui->usernameEdit->hide();
		passwordForOtp = ui->passwordEdit->text().toStdString();
		ui->passwordEdit->clear();
		ui->passwordEdit->setPlaceholderText(
			Str("Caffeine.Auth.VerificationPlaceholder"));
		ui->signInButton->setText(Str("Caffeine.Auth.Continue"));
		ui->newUserFooter->hide();
		ui->forgotPasswordLabel->setOpenExternalLinks(false);
		ui->forgotPasswordLabel->setText(
			QString(R"(<p>
			%1 <br/><a href="https://www.%2" style="color: rgb(0, 159, 224); 
			text-decoration: none">%3</a></p>)")
				.arg(QTStr("Caffeine.Auth.HavingProblems"),
				     getCaffeineURL,
				     QTStr("Caffeine.Auth.SendNewCode")));

		// Disable the signInButton upfront
		ui->signInButton->setEnabled(false);
		ui->passwordEdit->setMaxLength(otpLength);
		// Enable the signInButton when user has entered 6 digit opt
		connect(ui->passwordEdit, &QLineEdit::textChanged,
			[=](const QString &enteredOtp) {
				if (enteredOtp.length() == otpLength) {
					ui->signInButton->setEnabled(true);
				} else {
					ui->signInButton->setEnabled(false);
				}
			});
		return;
	case caff_ResultMfaOtpIncorrect:
		ui->passwordEdit->clear();
		ui->messageLabel->setText(
			Str("Caffeine.Auth.IncorrectVerification"));
		return;
	case caff_ResultUsernameRequired:
		ui->messageLabel->setText(
			Str("Caffeine.Auth.UsernameRequired"));
		return;
	case caff_ResultPasswordRequired:
		ui->messageLabel->setText(
			Str("Caffeine.Auth.PasswordRequired"));
		return;
	case caff_ResultInfoIncorrect:
		ui->messageLabel->setText(Str("Caffeine.Auth.IncorrectInfo"));
		return;
	case caff_ResultLegalAcceptanceRequired:
		ui->messageLabel->setText(
			Str("Caffeine.Auth.TosAcceptanceRequired"));
		return;
	case caff_ResultEmailVerificationRequired:
		ui->messageLabel->setText(
			Str("Caffeine.Auth.EmailVerificationRequired"));
		return;
	case caff_ResultFailure:
	default:
		ui->messageLabel->setText(Str("Caffeine.Auth.SigninFailed"));
		return;
	}
}

std::shared_ptr<Auth> CaffeineAuth::Login(QWidget *parent)
{
	static int once = addFonts();
	UNUSED_PARAMETER(once);
	const auto flags = Qt::WindowTitleHint | Qt::WindowSystemMenuHint |
			   Qt::WindowCloseButtonHint;

	QDialog dialog(parent, flags);
	auto ui = new Ui::CaffeineSignInDialog;
	ui->setupUi(&dialog);
	QIcon icon(":/caffeine/images/CaffeineLogo.svg");
	ui->logo->setPixmap(icon.pixmap(76, 66));

	// Set up text and point href depending on the env var
	ui->forgotPasswordLabel->setText(QString(R"(<p>
		forgot something?<br/><a href="https://www.%1/forgot-password" style="color: rgb(0, 159, 224); 
		text-decoration: none">reset your password</a></p>)")
						 .arg(getCaffeineURL));
	ui->newUserFooter->setText(
		QString(R"(New to Caffeine? <a href="https://www.%1/sign-up" style="color: white; 
		text-decoration: none; font-weight: bold">sign up</a>)")
			.arg(getCaffeineURL));

	// Don't highlight text boxes
	for (auto edit : dialog.findChildren<QLineEdit *>()) {
		edit->setAttribute(Qt::WA_MacShowFocusRect, false);
	}

	ui->messageLabel->clear();

	std::shared_ptr<CaffeineAuth> auth =
		std::make_shared<CaffeineAuth>(caffeineDef);

	std::string origPassword;
	connect(ui->signInButton, &QPushButton::clicked,
		[&](bool) { auth->TryAuth(ui, &dialog, origPassword); });

	// Only used for One-time-password "resend email" link. resending the
	// email is just attempting the sign-in without one-time password
	// included
	connect(ui->forgotPasswordLabel, &QLabel::linkActivated,
		[&](const QString &) {
			// Do this only for OTP
			if (!ui->newUserFooter->isVisible()) {
				auto username =
					ui->usernameEdit->text().toStdString();
				caff_signIn(auth->instance, username.c_str(),
					    origPassword.c_str(), nullptr);
			}
		});

	if (dialog.exec() == QDialog::Rejected)
		return nullptr;

	if (auth->GetChannelInfo())
		return auth;

	return nullptr;
}

std::string CaffeineAuth::GetUsername()
{
	return this->username;
}

static std::shared_ptr<Auth> CreateCaffeineAuth()
{
	return std::make_shared<CaffeineAuth>(caffeineDef);
}

static void DeleteCookies() {}

void RegisterCaffeineAuth()
{
	OAuth::RegisterOAuth(caffeineDef, CreateCaffeineAuth,
			     CaffeineAuth::Login, DeleteCookies);
}
