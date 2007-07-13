function demo_ep_clFull_mcmc
%DEMO_GP2CLASS    Classification problem demonstration for 2
%                 classes with EP and using MCMC for parameters.
%
%      Description
%      The demonstration program is based on synthetic two 
%      class data used by B.D. Ripley (Pattern Regocnition and
%      Neural Networks, 1996}. The data consists of 2-dimensional
%      vectors that are divided into to classes, labeled 0 or 1.
%      Each class has a bimodal distribution generated from equal
%      mixtures of Gaussian distributions with identical covariance
%      matrices. A Bayesian aprouch is used to find the decision
%      line and predict the classes of new data points.
%
%      The demonstration program does not sample for real, because
%      it would require so much time. The main sampling state is
%      commented out from the program and instead a saved network
%      structure is loaded and used to make predictions (see lines
%      143-146).
%

% Copyright (c) 2005 Jarno Vanhatalo, Jaakko Riihim�ki

% This software is distributed under the GNU General Public 
% License (version 2 or later); please refer to the file 
% License.txt, included with the software, for details.

% This demonstration is based on the dataset used in the book Pattern Recognition and
% Neural Networks by B.D. Ripley (1996) Cambridge University Press ISBN 0 521
% 46986 7

disp(' ')
disp(' The demonstration program is based on synthetic two ')
disp(' class data used by B.D. Ripley (Pattern Regocnition and')
disp(' Neural Networks, 1996}. The data consists of 2-dimensional')
disp(' vectors that are divided into to classes, labeled 0 or 1.')
disp(' Each class has a bimodal distribution generated from equal')
disp(' mixtures of Gaussian distributions with identical covariance')
disp(' matrices. A Gaussian process is used to find the decision')
disp(' line and predict the classes of new data points.')
disp(' ')
disp(' ')

  
% Load the data
S = which('demo_clFull');
L = strrep(S,'demo_clFull.m','demos/synth.tr');
x=load(L);
y=x(:,end);
y=y*2-1;
x(:,end)=[];
[n, nin] = size(x);

disp(' ')
disp(' First we create a Gaussian process for classification problem. ')
disp(' A Gaussian multivariate hierarchical prior with ARD is created')
disp(' for GP. ')
disp(' ')

% Create covariance functions
% Create covariance functions
gpcf1 = gpcf_sexp('init', nin, 'lengthScale', 1, 'magnSigma2', 0.2^2);
%gpcf1 = gpcf_sexp('init', nin, 'lengthScale', [1 1], 'magnSigma2', 0.2^2);

%gpcf1 = gpcf_exp('init', nin, 'lengthScale', 1, 'magnSigma2', 0.2^2);
%gpcf1 = gpcf_exp('init', nin, 'lengthScale', [1 1], 'magnSigma2', 0.2^2);

%gpcf1 = gpcf_matern32('init', nin, 'lengthScale', 1, 'magnSigma2', 0.2^2);
%gpcf1 = gpcf_matern32('init', nin, 'lengthScale', [1 1], 'magnSigma2', 0.2^2);

%gpcf1 = gpcf_matern52('init', nin, 'lengthScale', 1, 'magnSigma2', 0.2^2);
%gpcf1 = gpcf_matern52('init', nin, 'lengthScale', [1 1], 'magnSigma2', 0.2^2);

% Set the prior for the parameters of covariance functions 
gpcf1.p.lengthScale = gamma_p({3 7 3 7});
gpcf1.p.magnSigma2 = sinvchi2_p({0.05^2 0.5});

%gp = gp_init('init', nin, 'lh_2class', {gpcf1}, [], 'jitterSigmas', 1)   %{gpcf2}
gp = gp_init('init', 'FULL', nin, 'probit', {gpcf1}, [], 'jitterSigmas', 0.01)   %{gpcf2}
gp = gp_init('set', gp, 'latent_method', {'EP', x, y, 'hyper'});

% $$$ [e, edata, eprior] = gpep_e(gp_pak(gp,'hyper'), gp, x, y, 'hyper')
% $$$ 
% $$$ [g, gdata, gprior] = gpep_g(gp_pak(gp,'hyper'), gp, x, y, 'hyper')
% $$$ 
% $$$ gradcheck(gp_pak(gp,'hyper'), @gpep_e, @gpep_g, gp, x, y, 'hyper')
% $$$ 
% $$$ gradcheck(randn(size(gp_pak(gp,'hyper'))), @gpep_e, @gpep_g, gp, x, y, 'hyper')


disp(' ')
disp(' Find the point estimates for the parameters with early stop method. ')
disp(' ')

% Begin to sample. First set sampling options and then start sampling
disp(' ')
disp(' Now that the starting values are found we set the main sampling ')
disp(' options ')
disp(' ')

opt=gp_mcopt;
opt.repeat=1;
opt.nsamples=1;
opt.hmc_opt.steps=11;
opt.hmc_opt.stepadj=0.1;
opt.hmc_opt.nsamples=1;
hmc2('state', sum(100*clock))

opt.fh_e = @gpep_e;
opt.fh_g = @gpep_g;
%[r,g,rstate1]=gp_mc(opt, gp, x, y);

% Set the sampling options
opt.nsamples=50;
opt.repeat=1;
opt.hmc_opt.steps=3;
opt.hmc_opt.stepadj=0.1;
opt.hmc_opt.stepadj=0.1;
opt.hmc_opt.nsamples=1;
hmc2('state', sum(100*clock));

% Sample from the posterior. 

% NOTE! With EP it is faster to make the predictions while sampling 
% than afterwards. This can be done by given the test locations for
% gp_mc, which are here 'xstar'.
xt1=repmat(linspace(min(x(:,1)),max(x(:,1)),20)',1,20);
xt2=repmat(linspace(min(x(:,2)),max(x(:,2)),20)',1,20)';
xstar=[xt1(:) xt2(:)];

[r,g,rstate2]=gp_mc(opt, gp, x, y, xstar, []);

%rr=thin(r,50,8);
rr=thin(r,10);
p1 = mean(rr.p1, 1)';

% $$$ geyer_imse(rr.cf{1}.lengthScale)
% $$$ geyer_imse(rr.cf{1}.magnSigma2)

% ==============================================
% visualise predictive probability  p(ystar = 1)
figure, hold on;
n_pred=size(xstar,1);
h1=pcolor(reshape(xstar(:,1),20,20),reshape(xstar(:,2),20,20),reshape(p1,20,20))
set(h1, 'edgealpha', 0), set(h1, 'facecolor', 'interp')
colormap(repmat(linspace(1,0,64)', 1, 3).*repmat(ones(1,3), 64,1))
axis([-inf inf -inf inf]), axis off
plot(x(y==-1,1),x(y==-1,2),'o', 'markersize', 8, 'linewidth', 2);
plot(x(y==1,1),x(y==1,2),'rx', 'markersize', 8, 'linewidth', 2);
set(gcf, 'color', 'w'), title('predictive probability and training cases', 'fontsize', 14)

% Visualize the predictive variance
figure, hold on;
n_pred=size(xstar,1);
h1=pcolor(reshape(xstar(:,1),20,20),reshape(xstar(:,2),20,20),reshape(Varf,20,20))
set(h1, 'edgealpha', 0), set(h1, 'facecolor', 'interp')
colormap(repmat(linspace(1,0,64)', 1, 3).*repmat(ones(1,3), 64,1))
axis([-inf inf -inf inf]), axis off
plot(x(y==-1,1),x(y==-1,2),'o', 'markersize', 8, 'linewidth', 2);
plot(x(y==1,1),x(y==1,2),'rx', 'markersize', 8, 'linewidth', 2);
set(gcf, 'color', 'w'), title('marginal predictive latent variance', 'fontsize', 14)

% visualise predictive probability  p(ystar = 1) with contours
figure, hold on
[cs,h]=contour(reshape(xstar(:,1),20,20),reshape(xstar(:,2),20,20),reshape(p1,20,20),[0.025 0.25 0.5 0.75 0.975], 'linewidth', 3);
text_handle = clabel(cs,h);
set(text_handle,'BackgroundColor',[1 1 .6],'Edgecolor',[.7 .7 .7],'linewidth', 2, 'fontsize',14)
c1=[linspace(0,1,64)' 0*ones(64,1) linspace(1,0,64)'];
colormap(c1)
plot(x(y==1,1), x(y==1,2), 'rx', 'markersize', 8, 'linewidth', 2),
plot(x(y==-1,1), x(y==-1,2), 'bo', 'markersize', 8, 'linewidth', 2)
plot(xstar(:,1), xstar(:,2), 'k.'), axis([-inf inf -inf inf]), axis off
set(gcf, 'color', 'w'), title('predictive probability contours with training cases', 'fontsize', 14)


% =================================================================================
% test how well the network works for the test data. 
L = strrep(S,'demo_clFull.m','demos/synth.ts');
tx=load(L);
ty_temp=tx(:,end);
ty = 2*ty-1;
tx(:,end)=[];

[Eftest, Varftest, p1test] = ep_pred(gp, x, y, tx);

% calculate the percentage of misclassified points
missed = sum(abs(round(p1test)-ty_temp))/size(ty,1)*100

% ====================================================================================
% Plot the training and test cases in the same figure
figure, hold on;
set(text_handle,'BackgroundColor',[1 1 .6],'Edgecolor',[.7 .7 .7],'linewidth', 2, 'fontsize',14)
c1=[linspace(0,1,64)' 0*ones(64,1) linspace(1,0,64)'];
colormap(c1)
plot(x(y==-1,1),x(y==-1,2),'o', 'markersize', 8, 'linewidth', 2);
plot(x(y==1,1),x(y==1,2),'rx', 'markersize', 8, 'linewidth', 2);
plot(tx(ty==-1,1),tx(ty==-1,2),'go', 'markersize', 8, 'linewidth', 2);
plot(tx(ty==1,1),tx(ty==1,2),'cx', 'markersize', 8, 'linewidth', 2);
plot(xstar(:,1), xstar(:,2), 'k.'), axis([-inf inf -inf inf]), axis off
set(gcf, 'color', 'w'), title('training and test cases', 'fontsize', 14)

% visualise predictive probability  p(ystar = 1) with the test cases
figure, hold on;
n_pred=size(xstar,1);
h1=pcolor(reshape(xstar(:,1),20,20),reshape(xstar(:,2),20,20),reshape(p1,20,20))
set(h1, 'edgealpha', 0), set(h1, 'facecolor', 'interp')
colormap(repmat(linspace(1,0,64)', 1, 3).*repmat(ones(1,3), 64,1))
axis([-inf inf -inf inf]), axis off
plot(tx(ty==-1,1),tx(ty==-1,2),'o', 'markersize', 8, 'linewidth', 2);
plot(tx(ty==1,1),tx(ty==1,2),'rx', 'markersize', 8, 'linewidth', 2);
set(gcf, 'color', 'w'), title('predictive probability and test cases', 'fontsize', 14)











